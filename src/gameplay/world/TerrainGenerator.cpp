#include "TerrainGenerator.hpp"

#include <glm/vec3.hpp>

#include "gameplay/world/ChunkData.hpp"

TerrainGenerator::TerrainGenerator(ChunkData& chunk, const glm::ivec3& chunk_world_pos)
    : chunk_(chunk), chunk_world_pos_(chunk_world_pos) {
  chunk.blocks_ = std::make_unique<BlockTypeArray>();
}

void TerrainGenerator::GenerateSolid(BlockType block) {
  std::fill(chunk_.blocks_->begin(), chunk_.blocks_->end(), block);
  chunk_.block_count_ = ChunkVolume;
}

void TerrainGenerator::GenerateChecker(BlockType block) {
  ZoneScoped;
  glm::ivec3 iter;
  for (iter.y = 0; iter.y < ChunkLength; iter.y += 2) {
    for (iter.z = 0; iter.z < ChunkLength; iter.z += 2) {
      for (iter.x = 0; iter.x < ChunkLength; iter.x += 2) {
        SetBlock(iter, block);
      }
    }
  }
}

void TerrainGenerator::GenerateChecker(std::vector<BlockType>& blocks) {
  ZoneScoped;
  glm::ivec3 iter;
  for (iter.y = 0; iter.y < ChunkLength; iter.y += 2) {
    for (iter.z = 0; iter.z < ChunkLength; iter.z += 2) {
      for (iter.x = 0; iter.x < ChunkLength; iter.x += 2) {
        SetBlock(iter, blocks[rand() % blocks.size()]);
      }
    }
  }
}

void TerrainGenerator::GenerateLayers(BlockType block) {
  ZoneScoped;
  glm::ivec3 iter;
  for (iter.y = 0; iter.y < ChunkLength; iter.y += 2) {
    for (iter.z = 0; iter.z < ChunkLength; iter.z++) {
      for (iter.x = 0; iter.x < ChunkLength; iter.x++) {
        SetBlock(iter, block);
      }
    }
  }
}

void TerrainGenerator::GenerateLayers(std::vector<BlockType>& blocks) {
  ZoneScoped;
  glm::ivec3 iter;
  for (iter.y = 0; iter.y < ChunkLength; iter.y += 2) {
    for (iter.z = 0; iter.z < ChunkLength; iter.z++) {
      for (iter.x = 0; iter.x < ChunkLength; iter.x++) {
        SetBlock(iter, blocks[rand() % blocks.size()]);
      }
    }
  }
}

void TerrainGenerator::SetBlock(const glm::ivec3& pos, BlockType block) {
  (*chunk_.blocks_)[ChunkData::GetIndex(pos)] = block;
  chunk_.block_count_++;
}

void TerrainGenerator::SetBlock(int x, int y, int z, BlockType block) {
  (*chunk_.blocks_)[ChunkData::GetIndex(x, y, z)] = block;
  chunk_.block_count_++;
}
