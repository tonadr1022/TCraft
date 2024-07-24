#include "ChunkManager.hpp"

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
  // load the world data
}
