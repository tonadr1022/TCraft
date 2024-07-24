#include "ChunkManager.hpp"

#include "application/SettingsManager.hpp"
#include "gameplay/world/ChunkData.hpp"
#include "gameplay/world/ChunkUtil.hpp"

void ChunkManager::SetBlock(const glm::ivec3& pos, BlockType block) {
  auto it = chunk_map_.find(util::chunk::WorldToChunkPos(pos));
  EASSERT_MSG(it != chunk_map_.end(), "Set block in non existent chunk");
  it->second->data.SetBlock(util::chunk::WorldToPosInChunk(pos), block);
}

BlockType ChunkManager::GetBlock(const glm::ivec3& pos) {
  auto it = chunk_map_.find(util::chunk::WorldToChunkPos(pos));
  EASSERT_MSG(it != chunk_map_.end(), "Get block in non existent chunk");
  return it->second->data.GetBlock(util::chunk::WorldToPosInChunk(pos));
}

void ChunkManager::Update(double dt) {
  // only update if player changes chunk location
}

void ChunkManager::Init() {
  // Spiral iteration from 0,0
  constexpr static int Dx[] = {1, 0, -1, 0};
  constexpr static int Dy[] = {0, 1, 0, -1};
  int direction = 0;
  int step_radius = 1;
  int direction_steps_counter = 0;
  int turn_counter = 0;
  int load_len = SettingsManager::Get().load_distance * 2 + 1;
  glm::ivec2 iter{0, 0};
  for (int i = 0; i < load_len * load_len - 1; i++) {
    direction_steps_counter++;
    iter.x += Dx[direction];
    iter.y += Dy[direction];
    bool change_dir = direction_steps_counter == step_radius;
    direction = (direction + change_dir) % 4;
    direction_steps_counter *= !change_dir;
    turn_counter += change_dir;
    step_radius += change_dir * (1 - (turn_counter % 2));
  }
}
