#include "BlockDB.hpp"

#include "application/SettingsManager.hpp"
#include "gameplay/world/Block.hpp"
#include "util/JsonUtil.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

using json = nlohmann::json;

namespace {

const std::unordered_map<std::string, BlockType> BlockMap = {
    {"Air", BlockType::Air},     {"Plains Grass", BlockType::PlainsGrass},
    {"Dirt", BlockType::Dirt},   {"Stone", BlockType::Stone},
    {"Water", BlockType::Water}, {"Diamond Ore", BlockType::DiamondOre},
};

BlockType GetBlockTypeFromString(const std::string& blockName) {
  auto it = BlockMap.find(blockName);
  if (it != BlockMap.end()) {
    return it->second;
  }
  throw std::runtime_error("Unknown block type: " + blockName);
}

std::string GetStrAfterLastSlash(const std::string& str) {
  int idx = str.find_last_of('/');
  return str.substr(idx + 1);
}

}  // namespace

void BlockDB::Init(std::unordered_map<std::string, uint32_t>& name_to_idx) {
  ZoneScoped;
  block_data_db_.resize(BlockMap.size());
  block_mesh_data_db_.resize(BlockMap.size());
  std::unordered_map<std::string, BlockMeshData> model_name_to_mesh_data;
  LoadDefaultData(name_to_idx);
  LoadBlockModelData(name_to_idx, model_name_to_mesh_data);

  json block_data_arr = util::LoadJsonFile(GET_PATH("resources/data/block/block_data.json"));
  for (auto& block_data : block_data_arr) {
    BlockData data;
    data.name = block_data.value("name", block_defaults_.name);
    data.move_slow_multiplier =
        block_data.value("move_slow_multiplier", block_defaults_.move_slow_multiplier);
    data.emits_light = block_data.value("emits_light", block_defaults_.emits_light);
    BlockType type = GetBlockTypeFromString(data.name);
    block_data_db_[static_cast<int>(type)] = data;

    auto model = json_util::GetString(block_data, "model");
    block_mesh_data_db_[static_cast<int>(type)] =
        model_name_to_mesh_data[model.value_or(block_defaults_.model)];
  }
};

void BlockDB::LoadDefaultData(std::unordered_map<std::string, uint32_t>& name_to_idx) {
  ZoneScoped;
  json default_block_data = util::LoadJsonFile(GET_PATH("resources/data/block/default_block.json"));
  block_defaults_.name = default_block_data["name"].get<std::string>();
  // TODO: not necessary or use it, one or the other
  block_defaults_.model = default_block_data["model"].get<std::string>();
  auto default_mesh_data = LoadBlockModel("default.json", name_to_idx);
  EASSERT_MSG(default_mesh_data.has_value(), "Default mesh data failed to load");
  default_mesh_data_ = default_mesh_data.value();

  auto& properties = default_block_data["properties"];
  block_defaults_.move_slow_multiplier = properties["move_slow_multiplier"].get<float>();
  block_defaults_.emits_light = properties["emits_light"].get<bool>();
}

std::optional<BlockMeshData> BlockDB::LoadBlockModel(
    const std::string& model_filename, std::unordered_map<std::string, uint32_t>& name_to_idx) {
  ZoneScoped;
  std::string relative_path = GET_PATH("resources/data/block/model/" + model_filename);
  json model_obj = util::LoadJsonFile(relative_path);
  auto block_model_type = json_util::GetString(model_obj, "type");
  if (!block_model_type.has_value()) {
    return std::nullopt;
  }
  BlockMeshData block_mesh_data;

  auto tex_obj = json_util::GetObject(model_obj, "textures");
  if (!tex_obj.has_value()) {
    return std::nullopt;
  }
  if (!tex_obj.value().is_object()) {
    spdlog::error("textures key of model must be an object: {}", relative_path);
  }

  auto get_tex_idx = [this, &tex_obj, &name_to_idx,
                      &relative_path](const std::string& type) -> uint32_t {
    auto tex_name = json_util::GetString(tex_obj.value(), type);
    if (tex_name.has_value()) {
      return name_to_idx[tex_name.value()];
    }
    spdlog::error("model of type {} does not have required texture fields: {}", type,
                  relative_path);
    return block_defaults_.model_tex_index;
  };

  if (block_model_type == "block/all") {
    int tex_index = get_tex_idx("all");
    for (auto& val : block_mesh_data.texture_indices) {
      val = tex_index;
    }
  } else if (block_model_type == "block/top_bottom") {
    block_mesh_data.texture_indices[2] = get_tex_idx("top");
    block_mesh_data.texture_indices[3] = get_tex_idx("bottom");
    int side_tex_idx = get_tex_idx("side");
    block_mesh_data.texture_indices[0] = side_tex_idx;
    block_mesh_data.texture_indices[1] = side_tex_idx;
    block_mesh_data.texture_indices[4] = side_tex_idx;
    block_mesh_data.texture_indices[5] = side_tex_idx;
  } else if (block_model_type == "block/unique") {
    static constexpr const std::array<const char*, 6> SideStrs = {"posx", "negx", "posy", "posz",
                                                                  "negz"};
    for (int i = 0; i < 6; i++) {
      block_mesh_data.texture_indices[i] = get_tex_idx(SideStrs[i]);
    }
  } else {
    spdlog::error("Block model {} contains invalid type: {}", model_filename,
                  block_model_type.value());
    return std::nullopt;
  }
  return block_mesh_data;
}

void BlockDB::LoadBlockModelData(
    std::unordered_map<std::string, uint32_t>& name_to_idx,
    std::unordered_map<std::string, BlockMeshData>& model_name_to_mesh_data) {
  ZoneScoped;
  // Array of strings of block models to load
  json block_model_arr = util::LoadJsonFile(GET_PATH("resources/data/block/block_model_data.json"));
  // Load each block model file

  // TODO: use default texture index instead of 0 index
  BlockMeshData default_mesh_data{.texture_indices = {0, 0, 0, 0, 0, 0}};

  for (auto& item : block_model_arr) {
    std::string model_filename = item.get<std::string>();
    auto block_mesh_data = LoadBlockModel(model_filename, name_to_idx);
    std::string model_name = model_filename.substr(0, model_filename.find_last_of('.'));
    model_name_to_mesh_data.emplace("block/" + model_name,
                                    block_mesh_data.value_or(default_mesh_data));
  }
}
