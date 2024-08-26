#include "Terrain.hpp"

#include <nlohmann/json.hpp>

#include "gameplay/world/BlockDB.hpp"
#include "util/JsonUtil.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

BlockType BiomeLayer::GetBlock(float rand_neg_1_to_1) const {
  if (block_type_frequencies.size() == 1) {
    return block_types[0];
  }
  float sum = 0;
  for (size_t i = 0; i < block_type_frequencies.size(); i++) {
    sum += block_type_frequencies[i];
    if (sum >= ((rand_neg_1_to_1 + 1) / 2.0f)) return block_types[i];
  }
  EASSERT_MSG(0, "Unreachable");
  return block_types[0];
}

void Terrain::Load(const BlockDB& block_db) {
  biome_frequencies.clear();
  biomes.clear();

  id_stone = block_db.GetBlockData("stone")->id;
  id_sand = block_db.GetBlockData("sand")->id;

  ZoneScoped;
  std::vector<std::string> biome_names;
  auto fill_default_frequencies = [this, &biome_names]() {
    float freq = 1.0f / biome_names.size();
    for (size_t i = 0; i < biome_names.size(); i++) {
      biome_frequencies.emplace_back(freq);
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
        biome_frequencies.emplace_back(freq);
      }
    }
    if (biome_freqs.size() != biome_names.size()) {
      spdlog::error("Terrain load {}: Frequencies length {} is not same as biomes length", path,
                    biome_freqs.size(), biome_names.size());
      biome_frequencies.clear();
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
          layer.y_count = y_count;
        } else {
          layer.y_count = 1;
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
          layer.y_count = y_count;
        } else {
          layer.y_count = 1;
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

    int layer_y_sum = 0;
    for (const auto& layer : biome.layers) {
      layer_y_sum += layer.y_count;
    }
    biome.layer_y_sum = layer_y_sum;

    biomes.emplace_back(biome);
  }
}

bool Terrain::IsValid() const {
  float biome_freq_sum = std::accumulate(biome_frequencies.begin(), biome_frequencies.end(), 0.0f);
  constexpr const float kEpsilon = 0.0001f;
  if (abs(1.0f - biome_freq_sum) > kEpsilon) {
    return false;
  }
  for (const auto& biome : biomes) {
    for (const auto& layer : biome.layers) {
      float layer_block_freq_sum = std::accumulate(layer.block_type_frequencies.begin(),
                                                   layer.block_type_frequencies.end(), 0.0f);
      if (abs(1.0f - layer_block_freq_sum) > kEpsilon) {
        return false;
      }
    }
  }

  return true;
}
void Terrain::Write(const BlockDB& block_db) {
  std::vector<std::string> biome_names;
  biome_names.reserve(biomes.size());
  for (const auto& biome : biomes) {
    biome_names.emplace_back(biome.name);
  }
  nlohmann::json terrain_json = {{"biomes", biome_names}, {"frequencies", biome_frequencies}};
  for (const auto& biome : biomes) {
    std::filesystem::path path =
        GET_PATH("resources/data/terrain/biomes") / std::filesystem::path(biome.name + ".json");
    std::vector<nlohmann::json> layers;
    for (size_t layer_idx = 0; layer_idx < biome.layers.size(); layer_idx++) {
      const auto& layer = biome.layers[layer_idx];
      if (layer.block_types.size() == 1) {
        nlohmann::json layer_obj = {
            {"name", block_db.GetBlockData()[layer.block_types.front()].name},
            {"y_count", layer.y_count},
        };
        layers.emplace_back(layer_obj);
      } else {
        std::vector<std::string> names(layer.block_types.size());
        std::vector<float> freqs(layer.block_types.size());
        int i = 0;
        for (const BlockType block_type : layer.block_types) {
          names[i++] = block_db.GetBlockData()[block_type].name;
        }
        for (size_t block_idx = 0; block_idx < layer.block_types.size(); block_idx++) {
        }
        nlohmann::json layer_obj = {
            {"names", names},
            {"frequencies", layer.block_type_frequencies},
            {"y_count", layer.y_count},
        };
        layers.emplace_back(layer_obj);
      }
      nlohmann::json out_json = {
          {"name", biome.name}, {"formatted_name", biome.formatted_name}, {"layers", layers}};
      util::json::WriteJson(out_json, path);
    }
  }
}

const Biome& Terrain::GetBiome(float rand_neg_1_to_1) const {
  if (biomes.size() == 1) return biomes[0];
  float sum = 0;
  for (size_t i = 0; i < biome_frequencies.size(); i++) {
    sum += biome_frequencies[i];
    if (sum >= (rand_neg_1_to_1 + 1) / 2.0f) {
      return biomes[i];
    }
  }
  EASSERT_MSG(0, "Unreachable");
  return biomes[0];
}
