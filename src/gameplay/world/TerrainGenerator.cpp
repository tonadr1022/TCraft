#include "TerrainGenerator.hpp"

#include <FastNoise/FastNoise.h>

#include <glm/vec3.hpp>
#include <iostream>

#include "gameplay/world/ChunkData.hpp"
#include "gameplay/world/ChunkDef.hpp"

namespace {}  // namespace

TerrainGenerator::TerrainGenerator(ChunkData& chunk, const glm::ivec3& chunk_world_pos, int seed)
    : chunk_(chunk), chunk_world_pos_(chunk_world_pos), seed_(seed) {
  chunk.blocks_ = std::make_unique<BlockTypeArray>();
}

void TerrainGenerator::GenerateNoise(BlockType block, float frequency) {
  ZoneScoped;
  std::vector<float> out(ChunkArea);
  auto fn_simplex = FastNoise::New<FastNoise::Simplex>();
  auto fn_fractal = FastNoise::New<FastNoise::FractalFBm>();
  fn_fractal->SetSource(fn_simplex);
  fn_fractal->SetOctaveCount(5);
  fn_fractal->GenUniformGrid2D(out.data(), chunk_world_pos_.x * ChunkLength,
                               chunk_world_pos_.z * ChunkLength, ChunkLength, ChunkLength,
                               frequency, seed_);
  int height[ChunkArea];

  for (int i = 0; i < ChunkArea; i++) {
    height[i] = floor((out[i] + 1) * .5 * 22);
  }
  for (int y = 0; y < ChunkLength; y++) {
    int i = 0;
    for (int z = 0; z < ChunkLength; z++) {
      for (int x = 0; x < ChunkLength; x++, i++) {
        if (y <= height[i] + 5) {
          SetBlock(x, y, z, block);
        }
      }
    }
  }
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
