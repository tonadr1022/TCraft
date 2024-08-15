#include "TerrainGenerator.hpp"

#include <FastNoise/FastNoise.h>

#include <glm/vec3.hpp>

#include "EAssert.hpp"
#include "gameplay/world/ChunkData.hpp"
#include "gameplay/world/ChunkDef.hpp"
#include "gameplay/world/Terrain.hpp"

TerrainGenerator::TerrainGenerator(ChunkData& chunk, const glm::ivec3& chunk_world_pos, int seed,
                                   const Terrain& terrain)
    : chunk_(chunk), terrain_(terrain), chunk_world_pos_(chunk_world_pos), seed_(seed) {
  chunk.blocks_ = std::make_unique<BlockTypeArray>();
}

std::vector<int> TerrainGenerator::GetHeights(float frequency, float multiplier) const {
  ZoneScoped;
  std::vector<float> height_floats(ChunkArea);
  auto fn_simplex = FastNoise::New<FastNoise::Simplex>();
  auto fn_fractal = FastNoise::New<FastNoise::FractalRidged>();
  {
    ZoneScopedN("Get grid call");
    fn_fractal->SetSource(fn_simplex);
    fn_fractal->SetOctaveCount(2);
    fn_fractal->GenUniformGrid2D(height_floats.data(), chunk_world_pos_.x, chunk_world_pos_.z,
                                 ChunkLength, ChunkLength, frequency, seed_);
  }
  std::vector<int> height(ChunkArea);
  for (int i = 0; i < ChunkArea; i++) {
    height[i] = floor((height_floats[i] + 1) * .5 * multiplier);
  }
  return height;
}

void TerrainGenerator::GenerateBiome() {
  ZoneScoped;
  EASSERT_MSG(!terrain_.biomes.empty(), "Need biomes");
  auto get_block = [this](uint32_t y, uint32_t max_height) -> BlockType {
    const auto& biome = terrain_.biomes[0];
    if (y - chunk_world_pos_.y < max_height - biome.layer_y_sum) {
      return terrain_.stone;
    }
    uint32_t sum = 0;
    for (size_t i = 0; i < biome.layers.size(); i++) {
      sum += biome.layer_y_counts[i];
      if (sum > max_height - y - chunk_world_pos_.y) {
        return biome.layers[i].GetBlock();
      }
    }
    return terrain_.sand;
  };

  auto height = GetHeights(0.0013, MaxBlockHeight);
  for (int y = 0; y < ChunkLength; y++) {
    int i = 0;
    for (int z = 0; z < ChunkLength; z++) {
      for (int x = 0; x < ChunkLength; x++, i++) {
        if (y + chunk_world_pos_.y <= height[i]) {
          SetBlock(x, y, z, get_block(y, height[i]));
        }
      }
    }
  }
}

void TerrainGenerator::GenerateNoise(BlockType block, float frequency) {
  ZoneScoped;
  auto height = GetHeights(frequency, 22);
  for (int y = chunk_world_pos_.y; y < chunk_world_pos_.y + ChunkLength; y++) {
    int i = 0;
    for (int z = 0; z < ChunkLength; z++) {
      for (int x = 0; x < ChunkLength; x++, i++) {
        if (y <= height[i] - 2) {
          // TODO: real terrain gen
          SetBlock(x, y, z, 1);
        } else if (y <= height[i]) {
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
