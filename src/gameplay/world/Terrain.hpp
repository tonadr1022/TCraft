#pragma once

#include "gameplay/world/ChunkDef.hpp"

class BlockDB;

struct BiomeLayer {
  std::vector<BlockType> block_types;
  std::vector<float> block_type_frequencies;
};

struct Biome {
  std::string name;
  std::string formatted_name;
  std::vector<BiomeLayer> layers;
  std::vector<uint8_t> layer_y_counts;
};

struct Terrain {
  std::vector<Biome> biomes;
  std::vector<float> biom_frequencies;

  void Load(const BlockDB& block_db);
};
