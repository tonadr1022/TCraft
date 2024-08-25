#pragma once

#include "gameplay/world/ChunkDef.hpp"

class BlockDB;

struct BiomeLayer {
  std::vector<BlockType> block_types;
  std::vector<float> block_type_frequencies;
  [[nodiscard]] BlockType GetBlock(float rand_neg_1_to_1) const;
};

struct Biome {
  std::string name;
  std::string formatted_name;
  std::vector<BiomeLayer> layers;
  std::vector<uint8_t> layer_y_counts;
  uint32_t layer_y_sum{0};
};

struct Terrain {
  std::vector<Biome> biomes;
  std::vector<float> biome_frequencies;

  uint32_t stone;
  uint32_t sand;
  uint32_t default_id;

  void Load(const BlockDB& block_db);
  void Write(const BlockDB& block_db);
};
