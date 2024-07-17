#include "BlockDB.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "gameplay/world/Block.hpp"
#include "resource/ResourceLoader.hpp"
#include "util/LoadFile.hpp"
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

void BlockDB::LoadBlockModelData(std::unordered_map<std::string, BlockMeshData>& mesh_data_map) {
  ZoneScoped;
  // Array of strings of block models to load
  json block_model_arr = util::LoadJsonFile(GET_PATH("resources/data/block/block_model_data.json"));
  // Load each block model file
  auto& str_to_idx = ResourceLoader::block_texture_filename_to_tex_index_;

  // TODO: use default texture index instead of 0 index
  BlockMeshData default_mesh_data{.texture_indices = {0, 0, 0, 0, 0, 0}};

  for (auto& item : block_model_arr) {
    std::string block_model_filename = item.get<std::string>();
    std::string block_model_name =
        block_model_filename.substr(0, block_model_filename.find_last_of('.'));
    json model_obj =
        util::LoadJsonFile(GET_PATH("resources/data/block/model/" + block_model_filename));
    if (!model_obj.contains("type")) {
      spdlog::error("block model requires 'type': {}", block_model_filename);
    } else {
      auto block_model_type = GetStrAfterLastSlash(model_obj["type"].get<std::string>());
      BlockMeshData block_mesh_data;
      auto tex_obj = model_obj["textures"];
      if (block_model_type == "all") {
        auto texture_name = GetStrAfterLastSlash(tex_obj["all"].get<std::string>());
        int tex_index = str_to_idx[texture_name];
        for (auto& val : block_mesh_data.texture_indices) {
          val = tex_index;
        }
      } else if (block_model_type == "top_bottom") {
        auto top_tex_name = GetStrAfterLastSlash(tex_obj["top"].get<std::string>());
        block_mesh_data.texture_indices[2] = str_to_idx[top_tex_name];
        spdlog::info("toptexname {}, {}", top_tex_name, str_to_idx[top_tex_name]);
        auto bottom_tex_name = GetStrAfterLastSlash(tex_obj["bottom"].get<std::string>());
        block_mesh_data.texture_indices[3] = str_to_idx[bottom_tex_name];
        auto side_tex_name = GetStrAfterLastSlash(tex_obj["side"].get<std::string>());
        int side_tex_idx = str_to_idx[side_tex_name];
        block_mesh_data.texture_indices[0] = side_tex_idx;
        block_mesh_data.texture_indices[1] = side_tex_idx;
        block_mesh_data.texture_indices[4] = side_tex_idx;
        block_mesh_data.texture_indices[5] = side_tex_idx;
      } else if (block_model_type == "unique") {
        static constexpr const std::array<const char*, 6> SideStrs = {"posx", "negx", "posy",
                                                                      "posz", "negz"};
        for (int i = 0; i < 6; i++) {
          block_mesh_data.texture_indices[i] =
              str_to_idx[GetStrAfterLastSlash(tex_obj[SideStrs[i]].get<std::string>()) + ".png"];
        }
      } else {
        spdlog::error("Block model {} contains invalid type: {}", block_model_filename,
                      block_model_type);
        mesh_data_map.emplace(block_model_name, default_mesh_data);
        continue;
      }
      mesh_data_map.emplace(block_model_name, block_mesh_data);
    }
  }
}

void BlockDB::LoadData() {
  ZoneScoped;
  block_data_db_.resize(BlockMap.size());
  block_mesh_data_db_.resize(BlockMap.size());
  LoadDefaultData();
  std::unordered_map<std::string, BlockMeshData> mesh_data_map;
  LoadBlockModelData(mesh_data_map);
  for (const auto& el : mesh_data_map) {
    auto [a, b] = el;
    spdlog::info("{} {}", a, b.texture_indices[2]);
  }

  std::vector<bool> processed_block_types(BlockMap.size());

  std::ifstream block_file(GET_PATH("resources/data/block/block_data.json"));
  json block_data_arr = json::parse(block_file);

  for (auto& block_data : block_data_arr) {
    auto name = block_data.value("name", block_defaults_.name);
    BlockType type = GetBlockTypeFromString(name);
    auto model = block_data.value("model", block_defaults_.model);
    BlockMeshData mesh_data;

    // Can't have multiple
    EASSERT(!processed_block_types[static_cast<int>(type)]);
    processed_block_types[static_cast<int>(type)] = true;
    // block_data_db_[static_cast<int>(type)] = data;
  }
}
