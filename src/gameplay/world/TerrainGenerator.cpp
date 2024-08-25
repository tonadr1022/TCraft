#include "TerrainGenerator.hpp"

#include <FastNoise/FastNoise.h>

#include <cstddef>
#include <glm/vec3.hpp>

#include "EAssert.hpp"
#include "gameplay/world/Chunk.hpp"
#include "gameplay/world/ChunkDef.hpp"
#include "gameplay/world/Terrain.hpp"

namespace {

std::vector<int> GetHeightMap(float frequency, float multiplier, int start_x, int start_z,
                              int seed) {
  ZoneScoped;
  std::vector<float> height_floats(kChunkArea);
  auto fn_simplex = FastNoise::New<FastNoise::Simplex>();
  auto fn_fractal = FastNoise::New<FastNoise::FractalRidged>();
  fn_fractal->SetSource(fn_simplex);
  fn_fractal->SetOctaveCount(4);
  fn_fractal->GenUniformGrid2D(height_floats.data(), start_x, start_z, kChunkLength, kChunkLength,
                               frequency, seed);
  std::vector<int> height(kChunkArea);
  for (int i = 0; i < kChunkArea; i++) {
    height[i] = floor((height_floats[i] + 1) * .5 * multiplier);
  }
  return height;
}

std::vector<float> GetBiomeMap(float frequency, int start_x, int start_z, int seed) {
  std::vector<float> floats(static_cast<size_t>(kChunkArea));
  auto fn_simplex = FastNoise::New<FastNoise::Simplex>();
  auto fn_fbm = FastNoise::New<FastNoise::FractalFBm>();
  fn_fbm->SetSource(fn_simplex);
  fn_fbm->SetOctaveCount(2);
  fn_fbm->GenUniformGrid2D(floats.data(), start_x, start_z, kChunkLength, kChunkLength, frequency,
                           seed);
  return floats;
}

// std::vector<float> GetNoiseMap2D(float frequency, int start_x, int start_z, int seed) {
//   std::vector<float> noise_map(ChunkArea);
//   auto fn_noise = FastNoise::New<FastNoise::White>();
//   fn_noise->GenUniformGrid2D(noise_map.data(), start_x, start_z, ChunkLength, ChunkLength,
//                              frequency, seed);
//   return noise_map;
// }

std::vector<float> GetNoiseMap3D(float frequency, int start_x, int start_z, int seed, int height) {
  std::vector<float> noise_map(static_cast<size_t>(height * kChunkArea));
  auto fn_noise = FastNoise::New<FastNoise::White>();
  // fn_noise->GenUniformGrid2D(noise_map.data(), start_x, start_z, ChunkLength, ChunkLength,
  //                            frequency, seed);
  fn_noise->GenUniformGrid3D(noise_map.data(), start_x, 0, start_z, kChunkLength, height,
                             kChunkLength, frequency, seed);
  return noise_map;
}

}  // namespace

SingleChunkTerrainGenerator::SingleChunkTerrainGenerator(ChunkData& chunk,
                                                         const glm::ivec3& chunk_world_pos,
                                                         int seed, const Terrain& terrain)
    : chunk_(chunk), terrain_(terrain), chunk_world_pos_(chunk_world_pos), seed_(seed) {
  chunk.blocks_ = std::make_unique<BlockTypeArray>();
}

void TerrainGenerator::SetBlock(int x, int y, int z, BlockType block) {
  chunks_[y / kChunkLength]->data.SetBlock(x, y % kChunkLength, z, block);
}

void TerrainGenerator::GenerateBiome() {
  ZoneScoped;
  EASSERT_MSG(!terrain_.biomes.empty(), "Need biomes");
  // Timer timer;
  auto noise_map =
      GetNoiseMap3D(0.0013, chunk_world_pos_[0], chunk_world_pos_[1], seed_, kChunkLength);
  auto biome_map = GetBiomeMap(0.0013, chunk_world_pos_[0], chunk_world_pos_[1], seed_ + 1);
  auto get_block = [this, &noise_map, &biome_map](uint32_t y, uint32_t max_height,
                                                  int noise_map_idx, int xz) -> BlockType {
    const auto& biome = terrain_.GetBiome(biome_map[xz]);
    if (y <= max_height - biome.layer_y_sum) {
      return terrain_.id_stone;
    }

    uint32_t sum = 0;
    for (const auto& layer : biome.layers) {
      sum += layer.y_count;
      if (sum > max_height - y) {
        return layer.GetBlock(noise_map[noise_map_idx]);
      }
    }

    // Unreachable
    EASSERT(0);
    return terrain_.id_sand;
  };

  auto height_map = GetHeightMap(0.0013, static_cast<float>(kMaxBlockHeight) / 2,
                                 chunk_world_pos_[0], chunk_world_pos_[1], seed_);
  int j = 0;
  for (int y = 0; y < kChunkLength * kNumVerticalChunks; y++) {
    int i = 0;
    int xz = 0;
    for (int z = 0; z < kChunkLength; z++) {
      for (int x = 0; x < kChunkLength; x++, i++, j++, xz++) {
        if (y <= height_map[i]) {
          SetBlock(x, y, z, get_block(y, height_map[i], j % kChunkVolume, xz));
        }
      }
    }
  }
  // static double total = 0;
  // static int count = 0;
  // count++;
  // double curr = timer.ElapsedMS();
  // total += curr;
  // spdlog::info("{}", total / count);
}

void SingleChunkTerrainGenerator::GenerateNoise(BlockType block, float frequency) {
  ZoneScoped;
  auto height = GetHeightMap(frequency, 22, chunk_world_pos_.x, chunk_world_pos_.z, seed_);
  for (int y = chunk_world_pos_.y; y < chunk_world_pos_.y + kChunkLength; y++) {
    int i = 0;
    for (int z = 0; z < kChunkLength; z++) {
      for (int x = 0; x < kChunkLength; x++, i++) {
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
  for (const auto& chunk : chunks_) {
    chunk->data.blocks_ = std::make_unique<BlockTypeArray>();
    std::fill(chunk->data.blocks_->begin(), chunk->data.blocks_->end(), block);
    chunk->data.block_count_ = kChunkVolume;
  }
}

void SingleChunkTerrainGenerator::GenerateSolid(BlockType block) {
  std::fill(chunk_.blocks_->begin(), chunk_.blocks_->end(), block);
  chunk_.block_count_ = kChunkVolume;
}

void SingleChunkTerrainGenerator::GenerateChecker(BlockType block) {
  ZoneScoped;
  glm::ivec3 iter;
  for (iter.y = 0; iter.y < kChunkLength; iter.y += 2) {
    for (iter.z = 0; iter.z < kChunkLength; iter.z += 2) {
      for (iter.x = 0; iter.x < kChunkLength; iter.x += 2) {
        SetBlock(iter, block);
      }
    }
  }
}

void SingleChunkTerrainGenerator::GenerateChecker(std::vector<BlockType>& blocks) {
  ZoneScoped;
  glm::ivec3 iter;
  for (iter.y = 0; iter.y < kChunkLength; iter.y += 2) {
    for (iter.z = 0; iter.z < kChunkLength; iter.z += 2) {
      for (iter.x = 0; iter.x < kChunkLength; iter.x += 2) {
        SetBlock(iter, blocks[rand() % blocks.size()]);
      }
    }
  }
}

void SingleChunkTerrainGenerator::GenerateLayers(BlockType block) {
  ZoneScoped;
  glm::ivec3 iter;
  for (iter.y = 0; iter.y < kChunkLength; iter.y += 2) {
    for (iter.z = 0; iter.z < kChunkLength; iter.z++) {
      for (iter.x = 0; iter.x < kChunkLength; iter.x++) {
        SetBlock(iter, block);
      }
    }
  }
}

void SingleChunkTerrainGenerator::GenerateLayers(std::vector<BlockType>& blocks) {
  ZoneScoped;
  glm::ivec3 iter;
  for (iter.y = 0; iter.y < kChunkLength; iter.y += 2) {
    for (iter.z = 0; iter.z < kChunkLength; iter.z++) {
      for (iter.x = 0; iter.x < kChunkLength; iter.x++) {
        SetBlock(iter, blocks[rand() % blocks.size()]);
      }
    }
  }
}

void SingleChunkTerrainGenerator::SetBlock(const glm::ivec3& pos, BlockType block) {
  (*chunk_.blocks_)[ChunkData::GetIndex(pos)] = block;
  chunk_.block_count_++;
}

void SingleChunkTerrainGenerator::SetBlock(int x, int y, int z, BlockType block) {
  (*chunk_.blocks_)[ChunkData::GetIndex(x, y, z)] = block;
  chunk_.block_count_++;
}

TerrainGenerator::TerrainGenerator(
    const std::array<std::shared_ptr<Chunk>, kNumVerticalChunks>& chunks,
    const glm::ivec2& chunk_world_pos, int seed, const Terrain& terrain)
    : chunks_(chunks), terrain_(terrain), chunk_world_pos_(chunk_world_pos), seed_(seed) {}
