#pragma once

#include "gameplay/world/ChunkDef.hpp"

class BlockDB;

struct BiomeLayer {
  std::vector<BlockType> block_types;
  std::vector<float> block_type_frequencies;
  [[nodiscard]] BlockType GetBlock(float rand_neg_1_to_1) const;
  uint32_t y_count{1};
};

struct Biome {
  std::string name;
  std::string formatted_name;
  std::vector<BiomeLayer> layers;
  uint32_t layer_y_sum{0};
};

struct Terrain {
  std::vector<Biome> biomes;
  std::vector<float> biome_frequencies;
  [[nodiscard]] const Biome& GetBiome(float rand_neg_1_to_1) const;

  BlockType id_stone;
  BlockType id_sand;
  BlockType id_default;

  void Load(const BlockDB& block_db);
  void Write(const BlockDB& block_db);
  [[nodiscard]] bool IsValid() const;
};
