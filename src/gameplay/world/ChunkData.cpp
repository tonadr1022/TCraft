#include "ChunkData.hpp"

#include <glm/vec3.hpp>
#include <memory>

BlockType ChunkData::GetBlock(int x, int y, int z) const {
  if (blocks_ == nullptr) return 0;
  return (*blocks_)[GetIndex(x, y, z)];
}

BlockType ChunkData::GetBlock(const glm::ivec3& pos) const {
  if (blocks_ == nullptr) return 0;
  return (*blocks_)[GetIndex(pos)];
}

void ChunkData::SetBlock(const glm::ivec3& pos, BlockType block) {
  if (blocks_ == nullptr) blocks_ = std::make_unique<BlockTypeArray>();
  int index = GetIndex(pos);
  BlockType curr = (*blocks_)[index];
  block_count_ += (curr == 0 && block != 0);
  block_count_ -= (curr != 0 && block == 0);
  (*blocks_)[index] = block;
}

int ChunkData::GetBlockCount() const { return block_count_; }
