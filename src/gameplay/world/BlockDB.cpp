#include "BlockDB.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "gameplay/world/Block.hpp"
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
  std::ifstream f(GET_PATH("resources/data/block/block_defaults.json"));
  json default_json = json::parse(f);
  block_defaults_.name = default_json["name"].get<std::string>();
  block_defaults_.model = default_json["model"].get<std::string>();
  auto& properties = default_json["properties"];
  block_defaults_.move_slow_multiplier = properties["move_slow_multiplier"].get<float>();
  block_defaults_.emits_light = properties["emits_light"].get<bool>();
}

void BlockDB::LoadData() {
  LoadDefaultData();
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
