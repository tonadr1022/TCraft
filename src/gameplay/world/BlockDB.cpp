#include "BlockDB.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "gameplay/world/Block.hpp"
#include "resource/ResourceLoader.hpp"
#include "util/Paths.hpp"

using json = nlohmann::json;

BlockDB* BlockDB::instance_ = nullptr;
BlockDB& BlockDB::Get() { return *instance_; }
BlockDB::BlockDB() {
  EASSERT_MSG(instance_ == nullptr, "Cannot create two instances.");
  instance_ = this;
}

namespace {

const std::unordered_map<std::string, BlockType> BlockMap = {
    {"Air", BlockType::Air},     {"Plains Grass", BlockType::PlainsGrass},
    {"Dirt", BlockType::Dirt},   {"Stone", BlockType::Stone},
    {"Water", BlockType::Water},
};

BlockType GetBlockTypeFromString(const std::string& blockName) {
  auto it = BlockMap.find(blockName);
  if (it != BlockMap.end()) {
    return it->second;
  }
  throw std::runtime_error("Unknown block type: " + blockName);
}

}  // namespace

void BlockDB::LoadDefaultData() {
  ZoneScoped;
  std::ifstream f(GET_PATH("resources/data/block/block_defaults.json"));
  json default_json = json::parse(f);
  block_defaults_.name = default_json["name"].get<std::string>();
  block_defaults_.model = default_json["model"].get<std::string>();
  auto& properties = default_json["properties"];
  block_defaults_.move_slow_multiplier = properties["move_slow_multiplier"].get<float>();
  block_defaults_.emits_light = properties["emits_light"].get<bool>();
}

std::string GetStrAfterLastSlash(const std::string& str) {
  int idx = str.find_last_of('/');
  return str.substr(idx + 1);
}

void BlockDB::LoadBlockModelData() {
  ZoneScoped;
  // Array of strings of block models to load
  std::ifstream block_model_arr_file(GET_PATH("resources/data/block/block_model_data.json"));
  json block_model_arr = json::parse(block_model_arr_file);
  // Load each block model file
  for (auto& item : block_model_arr) {
    std::string block_model_filename = item.get<std::string>();
    std::ifstream model_file(GET_PATH("resources/data/block/model/" + block_model_filename));
    json obj = json::parse(model_file);
    if (!obj.contains("type")) {
      spdlog::error("block model requires 'type': {}", block_model_filename);
    } else {
      auto type = GetStrAfterLastSlash(obj["type"].get<std::string>());
      BlockMeshData block_mesh_data;
      if (type == "all") {
        auto texture_name =
            GetStrAfterLastSlash(obj["textures"]["all"].get<std::string>()) + ".png";
        // get tex index
        int tex_index = ResourceLoader::block_texture_filename_to_tex_index_[texture_name];
        for (auto& val : block_mesh_data.texture_indices) {
          val = tex_index;
        }
      } else if (type == "top_bottom") {
      } else if (type == "unique") {
      } else {
        spdlog::error("Block model {} contains invalid type: {}", block_model_filename, type);
      }
    }
  }
}

void BlockDB::LoadData() {
  ZoneScoped;
  LoadDefaultData();
  LoadBlockModelData();

  std::vector<bool> processed_block_types(BlockMap.size());

  std::ifstream block_file(GET_PATH("resources/data/block/block_data.json"));
  json block_data_arr = json::parse(block_file);
  block_data_db_.resize(BlockMap.size());
  block_mesh_data_db_.resize(BlockMap.size());

  for (auto& block_data : block_data_arr) {
    auto name = block_data.value("name", block_defaults_.name);
    BlockType type = GetBlockTypeFromString(name);
    auto model = block_data.value("model", block_defaults_.model);
    BlockMeshData mesh_data;

    EASSERT(!processed_block_types[static_cast<int>(type)]);
    processed_block_types[static_cast<int>(type)] = true;
    // BlockMeshData data{};
    // block_data_db_[static_cast<int>(type)] = data;
    spdlog::info("{} name", name);
  }
}
