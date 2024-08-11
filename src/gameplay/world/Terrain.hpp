#pragma once

#include "gameplay/world/ChunkDef.hpp"

class BlockDB;

struct BiomeLayer {
  std::vector<BlockType> block_types;
  std::vector<float> block_type_frequencies;
  [[nodiscard]] BlockType GetBlock() const;
};

struct Biome {
  std::string name;
  std::string formatted_name;
  std::vector<BiomeLayer> layers;
  std::vector<uint8_t> layer_y_counts;
  uint32_t layer_y_sum{3};
};

struct Terrain {
  std::vector<Biome> biomes;
  std::vector<float> biom_frequencies;

  uint32_t stone;
  uint32_t sand;

  void Load(const BlockDB& block_db);
};
