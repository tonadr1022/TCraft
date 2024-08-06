#include "ChunkManager.hpp"

#include <imgui.h>

#include "application/SettingsManager.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkData.hpp"
#include "gameplay/world/ChunkHelpers.hpp"
#include "gameplay/world/ChunkUtil.hpp"
#include "gameplay/world/TerrainGenerator.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Renderer.hpp"
#include "util/MemoryPool.hpp"
#include "util/Timer.hpp"

namespace {

dp::thread_pool thread_pool(std::thread::hardware_concurrency() - 4);

constexpr const int ChunkNeighborOffsets[27][3] = {
    {-1, -1, -1}, {-1, -1, 0}, {-1, -1, 1}, {-1, 0, -1}, {-1, 0, 0},  {-1, 0, 1}, {-1, 1, -1},
    {-1, 1, 0},   {-1, 1, 1},  {0, -1, -1}, {0, -1, 0},  {0, -1, 1},  {0, 0, -1}, {0, 0, 0},
    {0, 0, 1},    {0, 1, -1},  {0, 1, 0},   {0, 1, 1},   {1, -1, -1}, {1, -1, 0}, {1, -1, 1},
    {1, 0, -1},   {1, 0, 0},   {1, 0, 1},   {1, 1, -1},  {1, 1, 0},   {1, 1, 1},

};

}  // namespace

// TODO: find the best meshing memory pool size
ChunkManager::ChunkManager(BlockDB& block_db)
    : block_db_(block_db), meshing_mem_pool_(100), chunk_data_pool_(500) {}

void ChunkManager::SetBlock(const glm::ivec3& pos, BlockType block) {
  auto chunk_pos = util::chunk::WorldToChunkPos(pos);
  auto it = chunk_map_.find(chunk_pos);
  EASSERT_MSG(it != chunk_map_.end(), "Set block in non existent chunk");
  glm::ivec3 block_pos_in_chunk = util::chunk::WorldToPosInChunk(pos);
  it->second.data.SetBlock(block_pos_in_chunk, block);
  AddRelatedChunks(block_pos_in_chunk, chunk_pos, chunk_mesh_queue_immediate_);
}

BlockType ChunkManager::GetBlock(const glm::ivec3& pos) const {
  auto it = chunk_map_.find(util::chunk::WorldToChunkPos(pos));
  EASSERT_MSG(it != chunk_map_.end(), "Get block in non existent chunk");
  return it->second.data.GetBlock(util::chunk::WorldToPosInChunk(pos));
}

void ChunkManager::Update(double /*dt*/) {
  // TODO: implement ticks

  {
    ZoneScopedN("Process chunk terrain");
    for (const auto& pos : chunk_terrain_queue_) {
      // Chunk already exists at this point so no check
      thread_pool.enqueue_detach([this, pos] {
        ZoneScopedN("chunk terrain task");
        ChunkData* data = chunk_data_pool_.Allocate();
        TerrainGenerator gen{*data};
        gen.GenerateSolid(1);
        {
          std::lock_guard<std::mutex> lock(mutex_);
          finished_chunk_terrain_queue_.emplace(pos, data);
        }
      });
    }
    chunk_terrain_queue_.clear();
  }

  {
    ZoneScopedN("Process finsihed chunk terrain tasks");
    while (!finished_chunk_terrain_queue_.empty()) {
      const auto& task = finished_chunk_terrain_queue_.front();
      auto& chunk = chunk_map_.at(finished_chunk_terrain_queue_.front().first);
      chunk.data = *task.second;
      chunk.terrain_state = Chunk::State::Finished;
      finished_chunk_terrain_queue_.pop();
    }
  }

  // static glm::ivec3 center = util::chunk::WorldToChunkPos(center_);
  // static glm::ivec3 prev_center = center;
  // center = util::chunk::WorldToChunkPos(center_);
  // bool pos_changed = center == prev_center;
  {
    ZoneScopedN("Add to mesh queue");
    constexpr static int Dx[] = {1, 0, -1, 0};
    constexpr static int Dy[] = {0, 1, 0, -1};
    int direction = 0;
    int step_radius = 1;
    int direction_steps_counter = 0;
    int turn_counter = 0;
    // TODO: use load distance
    int load_len = (3) * 2 + 1;
    glm::ivec3 pos{0, 0, 0};
    for (int i = 0; i < load_len * load_len; i++) {
      Chunk& chunk = chunk_map_.at(pos);
      EASSERT_MSG(!(chunk.mesh_state == Chunk::State::Queued &&
                    chunk.terrain_state != Chunk::State::Finished),
                  "invalid combo");
      if (chunk.mesh_state != Chunk::State::NotFinished ||
          chunk.terrain_state != Chunk::State::Finished)
        continue;

      bool valid = true;
      for (const auto& off : ChunkNeighborOffsets) {
        if (chunk_map_.at({pos.x + off[0], pos.y + off[1], pos.z + off[2]}).terrain_state !=
            Chunk::State::Finished) {
          valid = false;
          break;
        }
      }
      if (valid) {
        chunk_mesh_queue_.emplace_back(pos);
        chunk.mesh_state = Chunk::State::Queued;
      }

      // iter
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
  // static glm::ivec3 prev_player_chunk_pos
  {
    ZoneScopedN("Process mesh chunks");
    // process remesh chunks
    for (const auto pos : chunk_mesh_queue_) {
      auto& chunk = chunk_map_.at(pos);
      EASSERT_MSG(chunk.mesh_state == Chunk::State::Queued, "Invalid chunk state for meshing");
      if (chunk.mesh.IsAllocated()) chunk.mesh.Free();
      // TODO: copy the neighbor chunks so the mesher can use them

      // Copy the data for immutability
      ChunkNeighborArray* a = meshing_mem_pool_.Allocate();
      PopulateChunkNeighbors(*a, pos);

      thread_pool.enqueue_detach([this, a, pos] {
        ChunkMesher mesher{block_db_.GetBlockData(), block_db_.GetMeshData()};
        std::vector<ChunkVertex> vertices;
        std::vector<uint32_t> indices;
        mesher.GenerateGreedy(*a, vertices, indices);
        {
          std::lock_guard<std::mutex> lock(mutex_);
          chunk_mesh_finished_queue_.emplace(vertices, indices, pos);
          chunk_map_.at(pos).mesh_state = Chunk::State::Finished;
          meshing_mem_pool_.Free(a);
        }
      });
    }
    chunk_mesh_queue_.clear();
  }

  {
    ZoneScopedN("Process finished mesh chunks");
    Timer timer;
    while (!chunk_mesh_finished_queue_.empty()) {
      if (timer.ElapsedMS() > 3) break;
      auto& task = chunk_mesh_finished_queue_.front();
      total_vertex_count_ += task.vertices.size();
      total_index_count_ += task.indices.size();
      num_mesh_creations_++;
      auto& chunk = chunk_map_.at(chunk_mesh_finished_queue_.front().pos);
      chunk.mesh.Allocate(task.vertices, task.indices);
      chunk.mesh_state = Chunk::State::Finished;
      chunk_mesh_finished_queue_.pop();
    }
  }

  {
    ZoneScopedN("Immediate chunk remesh");
    for (const auto& pos : chunk_mesh_queue_immediate_) {
      auto& chunk = chunk_map_.at(pos);
      // EASSERT_MSG(chunk.mesh_state == Chunk::State::Queued, "Invalid chunk state for meshing");

      bool can_mesh = true;
      for (const auto& offset : ChunkNeighborOffsets) {
        if (!chunk_map_.contains(pos + glm::ivec3{offset[0], offset[1], offset[2]})) {
          can_mesh = false;
          break;
        }
      }
      if (!can_mesh) continue;

      if (chunk.mesh.IsAllocated()) chunk.mesh.Free();
      ChunkNeighborArray* a = meshing_mem_pool_.Allocate();
      PopulateChunkNeighbors(*a, pos);
      ChunkMesher mesher{block_db_.GetBlockData(), block_db_.GetMeshData()};
      std::vector<ChunkVertex> vertices;
      std::vector<uint32_t> indices;
      mesher.GenerateGreedy(*a, vertices, indices);
      meshing_mem_pool_.Free(a);
      total_vertex_count_ += vertices.size();
      total_index_count_ += indices.size();
      num_mesh_creations_++;
      chunk.mesh.Allocate(vertices, indices);
      chunk.mesh_state = Chunk::State::Finished;
    }
    chunk_mesh_queue_immediate_.clear();
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
  for (size_t i = 1; i < block_data.size(); i++) {
    blocks.emplace_back(i);
  }

  constexpr static int Dx[] = {1, 0, -1, 0};
  constexpr static int Dy[] = {0, 1, 0, -1};
  int direction = 0;
  int step_radius = 1;
  int direction_steps_counter = 0;
  int turn_counter = 0;
  int load_len = 5 * 2 + 1;
  // int load_len = load_distance_ * 2 + 1;
  glm::ivec3 pos{0, 0, 0};
  for (int i = 0; i < load_len * load_len; i++) {
    chunk_map_.try_emplace(pos, pos);

    // TODO: either allow infinite chunks vertically or cap it out and optimize
    glm::ivec3 pos_neg_y = {pos.x, pos.y - 1, pos.z};
    glm::ivec3 pos_pos_y = {pos.x, pos.y + 1, pos.z};
    {
      chunk_map_.try_emplace(pos_pos_y, pos_pos_y);
      chunk_map_.try_emplace(pos_neg_y, pos_neg_y);
      chunk_map_.at(pos_neg_y).terrain_state = Chunk::State::Finished;
      chunk_map_.at(pos_pos_y).terrain_state = Chunk::State::Finished;
    }
    chunk_map_.try_emplace(pos, pos);
    chunk_terrain_queue_.emplace_back(pos);
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

void ChunkManager::PopulateChunkNeighbors(ChunkNeighborArray& neighbor_array,
                                          const glm::ivec3& pos) {
  ZoneScoped;
  for (const auto& off : ChunkNeighborOffsets) {
    neighbor_array[ChunkNeighborOffsetToIdx(off[0], off[1], off[2])] =
        &chunk_map_.at({pos.x + off[0], pos.y + off[1], pos.z + off[2]}).data.GetBlocks();
    // TODO: remove, not relevant to the function
    EASSERT(chunk_map_.at({pos.x + off[0], pos.y + off[1], pos.z + off[2]}).terrain_state ==
            Chunk::State::Finished);
  }
}

void ChunkManager::AddRelatedChunks(const glm::ivec3& block_pos_in_chunk,
                                    const glm::ivec3& chunk_pos,
                                    std::unordered_set<glm::ivec3>& chunk_set) {
  ZoneScoped;
  glm::ivec3 chunks_to_add[8];  // at most 8 chunks are related to a block
  glm::ivec3 temp;  // temp variable to store the chunk to add (calculate offset from chunk pos)
  int num_chunks_to_add = 1;  // always add the chunk the block is in
  int size;
  chunks_to_add[0] = chunk_pos;  // always add the chunk the block is in

  // iterate over each axis
  for (int axis = 0; axis < 3; axis++) {
    // if block is on edge of the axis, add the chunko on the other side of the edge
    if (block_pos_in_chunk[axis] == 0) {
      size = num_chunks_to_add;
      for (int i = 0; i < size; i++) {
        temp = chunks_to_add[i];  // works since only doing one axis at a time
        temp[axis]--;             // decrement chunk pos on the axis
        chunks_to_add[num_chunks_to_add++] = temp;
      }
    }

    if (block_pos_in_chunk[axis] == ChunkLength - 1) {
      size = num_chunks_to_add;
      for (int i = 0; i < size; i++) {
        temp = chunks_to_add[i];
        temp[axis]++;
        chunks_to_add[num_chunks_to_add++] = temp;
      }
    }
  }

  // add the chunks to the set
  for (int i = 0; i < num_chunks_to_add; i++) {
    chunk_set.insert(chunks_to_add[i]);
  }
}
