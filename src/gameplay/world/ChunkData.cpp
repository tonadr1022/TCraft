#include "ChunkData.hpp"

#include <glm/vec3.hpp>

BlockType ChunkData::GetBlock(const glm::ivec3& pos) { return blocks_[GetIndex(pos)]; }

int ChunkData::GetIndex(const glm::ivec3& pos) { return GetIndex(pos.x, pos.y, pos.z); }

int ChunkData::GetIndex(int x, int y, int z) { return x + ChunkLength * (z + ChunkLength * y); }

void ChunkData::SetBlock(const glm::ivec3& pos, BlockType block) {
  int index = GetIndex(pos);
  BlockType curr = blocks_[index];
  non_air_block_count_ += (curr == BlockType::Air && block != BlockType::Air);
  non_air_block_count_ -= (curr != BlockType::Air && block == BlockType::Air);
  blocks_[index] = block;
}

bool ChunkData::IsOutOfBounds(const glm::ivec3& pos) { return IsOutOfBounds(pos.x, pos.y, pos.z); }

bool ChunkData::IsOutOfBounds(int x, int y, int z) {
  return x >= ChunkLength || x < 0 || y >= ChunkLength || y < 0 || z >= ChunkLength || z < 0;
}
