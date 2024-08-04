#include "ChunkManager.hpp"

#include <imgui.h>

#include "application/SettingsManager.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkData.hpp"
#include "gameplay/world/ChunkUtil.hpp"
#include "gameplay/world/TerrainGenerator.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Renderer.hpp"

ChunkManager::ChunkManager(BlockDB& block_db) : block_db_(block_db) {}

void ChunkManager::SetBlock(const glm::ivec3& pos, BlockType block) {
  auto chunk_pos = util::chunk::WorldToChunkPos(pos);
  auto it = chunk_map_.find(chunk_pos);
  EASSERT_MSG(it != chunk_map_.end(), "Set block in non existent chunk");
  it->second.GetData().SetBlock(util::chunk::WorldToPosInChunk(pos), block);
  remesh_chunks_.emplace_back(chunk_pos);
}

BlockType ChunkManager::GetBlock(const glm::ivec3& pos) const {
  auto it = chunk_map_.find(util::chunk::WorldToChunkPos(pos));
  EASSERT_MSG(it != chunk_map_.end(), "Get block in non existent chunk");
  return it->second.GetBlock(util::chunk::WorldToPosInChunk(pos));
}

void ChunkManager::Update(double /*dt*/) {
  // TODO: implement ticks
  for (const auto& pos : remesh_chunks_) {
    // Chunk already exists at this point so no check
    auto& chunk = chunk_map_.at(pos);
    chunk.mesh_.Free();

    // TODO: use a vector for "to mesh chunks";
    ChunkMesher mesher{block_db_.GetBlockData(), block_db_.GetMeshData()};
    // TODO: separate  vertices and indices from here so multithreading is possible
    std::vector<ChunkVertex> vertices;
    std::vector<uint32_t> indices;
    mesher.GenerateNaive(chunk.GetData(), vertices, indices);
    chunk.mesh_.Allocate(vertices, indices);
  }
  remesh_chunks_.clear();
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
    chunk_map_.try_emplace(pos, pos);
    Chunk& chunk = chunk_map_.at(pos);

    TerrainGenerator gen{chunk.GetData()};
    gen.GenerateChecker(blocks);
    ChunkMesher mesher{block_db_.GetBlockData(), block_db_.GetMeshData()};
    // TODO: separate  vertices and indices from here so multithreading is possible
    std::vector<ChunkVertex> vertices;
    std::vector<uint32_t> indices;
    mesher.GenerateNaive(chunk.GetData(), vertices, indices);
    chunk.mesh_.Allocate(vertices, indices);

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
  }
}

bool ChunkManager::BlockPosExists(const glm::ivec3& world_pos) const {
  return chunk_map_.contains(util::chunk::WorldToChunkPos(world_pos));
}
