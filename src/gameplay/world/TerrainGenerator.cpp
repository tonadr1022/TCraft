#include "TerrainGenerator.hpp"

#include <glm/vec3.hpp>

#include "Block.hpp"
#include "gameplay/world/ChunkData.hpp"

void TerrainGenerator::GenerateSolid(ChunkData& chunk, BlockType block) {
  std::fill(chunk.blocks_.begin(), chunk.blocks_.end(), block);
}

void TerrainGenerator::GenerateChecker(ChunkData& chunk, BlockType block) {
  ZoneScoped;
  glm::ivec3 iter;
  for (iter.y = 0; iter.y < ChunkLength; iter.y += 2) {
    for (iter.z = 0; iter.z < ChunkLength; iter.z += 2) {
      for (iter.x = 0; iter.x < ChunkLength; iter.x += 2) {
        chunk.blocks_[ChunkData::GetIndex(iter)] = block;
      }
    }
  }
}

void TerrainGenerator::GenerateChecker(ChunkData& chunk, std::vector<BlockType>& blocks) {
  ZoneScoped;
  glm::ivec3 iter;
  for (iter.y = 0; iter.y < ChunkLength; iter.y += 2) {
    for (iter.z = 0; iter.z < ChunkLength; iter.z += 2) {
      for (iter.x = 0; iter.x < ChunkLength; iter.x += 2) {
        chunk.blocks_[ChunkData::GetIndex(iter)] = blocks[rand() % blocks.size()];
      }
    }
  }
}

void TerrainGenerator::GeneratePyramid(ChunkData& chunk, std::vector<BlockType>& blocks) {
  ZoneScoped;
  glm::ivec3 iter = {(ChunkLength - 1) / 2, 0, (ChunkLength - 1) / 2};
}
