#include "ChunkData.hpp"

#include <glm/vec3.hpp>

BlockType ChunkData::GetBlock(const glm::ivec3& pos) const { return blocks_[GetIndex(pos)]; }

void ChunkData::SetBlock(const glm::ivec3& pos, BlockType block) {
  int index = GetIndex(pos);
  BlockType curr = blocks_[index];
  non_air_block_count_ += (curr == 0 && block != 0);
  non_air_block_count_ -= (curr != 0 && block == 0);
  blocks_[index] = block;
}
