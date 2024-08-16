#include "Terrain.hpp"

#include <nlohmann/json.hpp>

#include "gameplay/world/BlockDB.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

BlockType BiomeLayer::GetBlock() const {
  float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
  float s = 0;
  for (size_t i = 0; i < block_type_frequencies.size(); i++) {
    s += block_type_frequencies[i];
    if (s >= r) return block_types[i];
  }
  spdlog::error("unintended {} {}", s, r);
  return block_types[0];
}

void Terrain::Load(const BlockDB& block_db) {
  // Should never fail
  stone = block_db.GetBlockData("stone")->id;
  sand = block_db.GetBlockData("sand")->id;

  ZoneScoped;
  std::vector<std::string> biome_names;
  auto fill_default_frequencies = [this, &biome_names]() {
    float freq = 1.0f / biome_names.size();
    for (size_t i = 0; i < biome_names.size(); i++) {
      biom_frequencies.emplace_back(freq);
    }
  };

  std::string path = GET_PATH("resources/data/terrain/default_terrain.json");
  {
    ZoneScopedN("Load terrain data");
    auto data = util::LoadJsonFile(path);
    for (const auto& biome : data["biomes"]) {
      biome_names.emplace_back(biome);
    }
    auto biome_freqs = data["frequencies"];
    if (!biome_freqs.is_array()) {
      fill_default_frequencies();
    } else {
      for (const float freq : biome_freqs) {
        biom_frequencies.emplace_back(freq);
      }
    }
    if (biome_freqs.size() != biome_names.size()) {
      spdlog::error("Terrain load {}: Frequencies length {} is not same as biomes length", path,
                    biome_freqs.size(), biome_names.size());
      biom_frequencies.clear();
      fill_default_frequencies();
    }
  }
  for (auto& biome_name : biome_names) {
    std::string biome_path = GET_PATH("resources/data/terrain/biomes/") + biome_name + ".json";
    auto biome_data = util::LoadJsonFile(biome_path);
    if (!biome_data.is_object()) {
      spdlog::error("Biome {}: not an object", biome_path);
      continue;
    }
    Biome biome;
    biome.name = biome_data.value("name", "");
    if (biome.name.empty()) {
      spdlog::error("Biome {}: no biome name", biome_path);
      continue;
    }
    biome.formatted_name = biome_data.value("name", biome.name);

    auto layer_arr = biome_data["layers"];
    if (!layer_arr.is_array()) {
      spdlog::error("Biome {}: no layers array");
      continue;
    }
    const BlockData* default_block_data = block_db.GetBlockData("default_block");
    EASSERT_MSG(default_block_data != nullptr, "Failed to laod default block data");
    for (auto& layer_data : layer_arr) {
      if (!layer_data.is_object()) {
        spdlog::error("Layer data is not object in biome {}", biome.formatted_name);
        continue;
      }
      BiomeLayer layer;
      if (layer_data.contains("names")) {
        for (auto& name : layer_data["names"]) {
          if (!name.is_string()) {
            spdlog::error("invalid name type in biome {}", biome.formatted_name);
          }

          const BlockData* block_data = block_db.GetBlockData(name);
          if (!block_data) {
            spdlog::error("block name {} not found", std::string(name));
            layer.block_types.emplace_back(default_block_data->id);
          } else {
            layer.block_types.emplace_back(block_data->id);
          }
        }

        for (auto& freq : layer_data["frequencies"]) {
          if (!freq.is_number_float()) {
            spdlog::error("invalid frequency type in biome {}", biome.formatted_name);
            layer.block_type_frequencies.emplace_back(0);
          } else {
            layer.block_type_frequencies.emplace_back(static_cast<float>(freq));
          }
        }

        auto fill_default_frequencies = [&layer]() {
          layer.block_type_frequencies.clear();
          float freq = 1.0f / layer.block_types.size();
          for (size_t i = 0; i < layer.block_types.size(); i++) {
            layer.block_type_frequencies.emplace_back(freq);
          }
        };

        if (layer.block_type_frequencies.size() != layer.block_types.size()) {
          spdlog::error("Block Type size not equal frequency size in layer in biome {}",
                        biome.formatted_name);
          fill_default_frequencies();
        }
        float sum = 0;
        for (const float freq : layer.block_type_frequencies) {
          sum += freq;
        }
        if (sum < 0.99 || sum > 1.001) {
          spdlog::error("Invalid frequency sum {} for layer in biome {}", sum,
                        biome.formatted_name);
          fill_default_frequencies();
        }
        auto y_count = layer_data["y_count"];
        if (y_count.is_number_unsigned()) {
          biome.layer_y_counts.emplace_back(y_count);
        } else {
          biome.layer_y_counts.emplace_back(1);
        }
        biome.layers.emplace_back(layer);
      } else if (layer_data.contains("name")) {
        auto name = layer_data["name"];
        if (!name.is_string()) {
          spdlog::error("invalid name type in biome {}", biome.formatted_name);
        }

        const BlockData* block_data = block_db.GetBlockData(name);
        if (!block_data) {
          spdlog::error("block name {} not found", std::string(name));
          layer.block_types.emplace_back(default_block_data->id);
        } else {
          layer.block_types.emplace_back(block_data->id);
        }

        // Only one block in the layer, so it has 100% freq
        layer.block_type_frequencies.emplace_back(1);
        auto y_count = layer_data["y_count"];
        if (y_count.is_number_unsigned()) {
          biome.layer_y_counts.emplace_back(y_count);
        } else {
          biome.layer_y_counts.emplace_back(1);
        }
        biome.layers.emplace_back(layer);
      } else {
        spdlog::error("Biome {}: all layers must contain 'names' or 'name' field");
      }
    }
    if (biome.layers.empty()) {
      spdlog::info("Biome {}: must have at least one layer");
      continue;
    }
    biomes.emplace_back(biome);
  }
}
