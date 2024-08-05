#include "ChunkManager.hpp"

#include <imgui.h>

#include "application/SettingsManager.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkData.hpp"
#include "gameplay/world/ChunkUtil.hpp"
#include "gameplay/world/TerrainGenerator.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Renderer.hpp"

namespace {

dp::thread_pool thread_pool;

}
ChunkManager::ChunkManager(BlockDB& block_db) : block_db_(block_db) {}

void ChunkManager::SetBlock(const glm::ivec3& pos, BlockType block) {
  auto chunk_pos = util::chunk::WorldToChunkPos(pos);
  auto it = chunk_map_.find(chunk_pos);
  EASSERT_MSG(it != chunk_map_.end(), "Set block in non existent chunk");
  it->second.data.SetBlock(util::chunk::WorldToPosInChunk(pos), block);
  immediate_remesh_chunk_task_positions_.emplace_back(chunk_pos);
}

BlockType ChunkManager::GetBlock(const glm::ivec3& pos) const {
  auto it = chunk_map_.find(util::chunk::WorldToChunkPos(pos));
  EASSERT_MSG(it != chunk_map_.end(), "Get block in non existent chunk");
  return it->second.data.GetBlock(util::chunk::WorldToPosInChunk(pos));
}

void ChunkManager::Update(double /*dt*/) {
  // TODO: implement ticks

  {
    ZoneScopedN("Process chunk terrain ");
    for (const auto pos : chunk_terrain_task_positions_) {
      // Chunk already exists at this point so no check
      thread_pool.enqueue_detach([this, pos] {
        ZoneScopedN("chunk terrain task");
        ChunkData data;
        TerrainGenerator gen{data};
        gen.GenerateLayers(1);
        {
          std::lock_guard<std::mutex> lock(queue_mutex_);
          finished_chunk_terrain_tasks_.emplace(pos, data);
        }
      });
    }
    chunk_terrain_task_positions_.clear();
  }

  {
    ZoneScopedN("Process finsihed chunk terrain tasks");
    while (!finished_chunk_terrain_tasks_.empty()) {
      const auto& task = finished_chunk_terrain_tasks_.front();
      auto& chunk = chunk_map_.at(finished_chunk_terrain_tasks_.front().first);
      chunk.data = task.second;
      // remesh_chunk_tasks_positions_.emplace_back(task.first);
      immediate_remesh_chunk_task_positions_.emplace_back(task.first);
      // meshing_candidate_positions_.emplace_back(task.first);
      finished_chunk_terrain_tasks_.pop();
    }
  }

  {
    ZoneScopedN("Process remesh chunks");
    // process remesh chunks
    for (const auto pos : remesh_chunk_tasks_positions_) {
      // Chunk already exists at this point so no check
      auto& chunk = chunk_map_.at(pos);
      if (chunk.mesh.IsAllocated()) chunk.mesh.Free();
      // Copy the data for immutability
      // TODO: copy the neighbor chunks so the mesher can use them
      ChunkData data = chunk.data;
      thread_pool.enqueue_detach([this, data, pos] {
        ChunkMesher mesher{block_db_.GetBlockData(), block_db_.GetMeshData()};
        std::vector<ChunkVertex> vertices;
        std::vector<uint32_t> indices;
        mesher.GenerateNaive(data, vertices, indices);
        {
          std::lock_guard<std::mutex> lock(queue_mutex_);
          finished_chunk_mesh_tasks_.emplace(vertices, indices, pos);
        }
      });
    }
    remesh_chunk_tasks_positions_.clear();
  }

  {
    ZoneScopedN("Process finished mesh chunks");
    while (!finished_chunk_mesh_tasks_.empty()) {
      auto& task = finished_chunk_mesh_tasks_.front();
      total_vertex_count_ += task.vertices.size();
      total_index_count_ += task.indices.size();
      num_mesh_creations_++;
      auto& chunk = chunk_map_.at(finished_chunk_mesh_tasks_.front().pos);
      spdlog::info("{} {} {}", chunk.GetPos().x, chunk.GetPos().y, chunk.GetPos().z);
      chunk.mesh.Allocate(task.vertices, task.indices);
      finished_chunk_mesh_tasks_.pop();
    }
  }

  {
    for (const auto& pos : immediate_remesh_chunk_task_positions_) {
      auto& chunk = chunk_map_.at(pos);
      if (chunk.mesh.IsAllocated()) chunk.mesh.Free();
      std::vector<ChunkVertex> vertices;
      std::vector<uint32_t> indices;
      ChunkMesher mesher{block_db_.GetBlockData(), block_db_.GetMeshData()};
      mesher.GenerateNaive(chunk.data, vertices, indices);
      total_vertex_count_ += vertices.size();
      total_index_count_ += indices.size();
      num_mesh_creations_++;
      chunk.mesh.Allocate(vertices, indices);
    }
    immediate_remesh_chunk_task_positions_.clear();
    ZoneScopedN("Immediate chunk remesh");
  }
}

void ChunkManager::Init() {
  auto settings = SettingsManager::Get().LoadSetting("chunk_manager");
  load_distance_ = settings.value("load_distance", 16);
  // Spiral iteration from 0,0

  // gather vector of blocks
  std::vector<BlockType> blocks;
  const auto& block_data = block_db_.GetBlockData();
  blocks.reserve(block_data.size());
  spdlog::info("block_data size {}", block_data.size());
  for (size_t i = 1; i < block_data.size(); i++) {
    blocks.emplace_back(i);
  }

  constexpr static int Dx[] = {1, 0, -1, 0};
  constexpr static int Dy[] = {0, 1, 0, -1};
  int direction = 0;
  int step_radius = 1;
  int direction_steps_counter = 0;
  int turn_counter = 0;
  int load_len = 1 * 2 + 1;
  // int load_len = load_distance_ * 2 + 1;
  glm::ivec3 pos{0, 0, 0};
  for (int i = 0; i < load_len * load_len; i++) {
    // for (int i = 0; i < 5; i++) {
    //   pos.y = i;
    chunk_map_.try_emplace(pos, pos);
    chunk_terrain_task_positions_.emplace_back(pos);
    // }
    direction_steps_counter++;
    pos.x += Dx[direction];
    pos.z += Dy[direction];
    bool change_dir = direction_steps_counter == step_radius;
    direction = (direction + change_dir) % 4;
    direction_steps_counter *= !change_dir;
    turn_counter += change_dir;
    step_radius += change_dir * (1 - (turn_counter % 2));
  }
}

ChunkManager::~ChunkManager() {
  nlohmann::json j = {{"load_distance", load_distance_}};
  SettingsManager::Get().SaveSetting(j, "chunk_manager");
}

void ChunkManager::OnImGui() {
  if (ImGui::CollapsingHeader("Chunk Manager", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::SliderInt("Load Distance", &load_distance_, 1, 32);
    if (num_mesh_creations_ > 0) {
      ImGui::Text("Avg vertices: %i", total_vertex_count_ / num_mesh_creations_);
      ImGui::Text("Avg indices: %i", total_index_count_ / num_mesh_creations_);
      ImGui::Text("Total vertices: %i", total_vertex_count_);
      ImGui::Text("Total indices: %i", total_index_count_);
    }
  }
}

bool ChunkManager::BlockPosExists(const glm::ivec3& world_pos) const {
  return chunk_map_.contains(util::chunk::WorldToChunkPos(world_pos));
}
