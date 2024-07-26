#include "BlockDB.hpp"

#include "application/SettingsManager.hpp"
#include "pch.hpp"
#include "util/JsonUtil.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

using json = nlohmann::json;

const std::vector<BlockMeshData>& BlockDB::GetMeshData() const { return block_mesh_data_; };
const std::vector<BlockData>& BlockDB::GetBlockData() const { return block_data_arr_; };

void BlockDB::Init(std::unordered_map<std::string, uint32_t>& name_to_idx) {
  ZoneScoped;
  std::unordered_map<std::string, BlockMeshData> model_name_to_mesh_data;
  LoadDefaultData(name_to_idx);
  block_defaults_loaded_ = true;
  LoadBlockModelData(name_to_idx, model_name_to_mesh_data);

  json block_data_arr = util::LoadJsonFile(GET_PATH("resources/data/block/block_data.json"));
  // plus 1 due to air block
  int num_block_types = block_data_arr.size() + 1;
  block_data_arr_.resize(num_block_types);
  block_mesh_data_.resize(num_block_types);
  block_model_names_.resize(num_block_types);

  // ensures that multiple IDs cannot exist
  std::vector<bool> processed(num_block_types);

  for (auto& block_data : block_data_arr) {
    BlockData data;
    data.name = block_data.value("name", block_defaults_.name);
    data.id = block_data.value("id", 0);
    if (block_data.contains("properties")) {
      auto properties = block_data["properties"];
      data.move_slow_multiplier =
          properties.value("move_slow_multiplier", block_defaults_.move_slow_multiplier);
      data.emits_light = properties.value("emits_light", block_defaults_.emits_light);
    }
    EASSERT_MSG(data.id <= num_block_types && data.id > 0, "Invalid data id");
    EASSERT_MSG(!processed[data.id], "Duplicate Data id");
    processed[data.id] = true;
    block_data_arr_[data.id] = data;

    std::string model_name = block_data.value("model", block_defaults_.model);
    block_mesh_data_[data.id] = model_name_to_mesh_data[model_name];
    block_model_names_[data.id] = model_name;
  }

  // Set air data
  block_data_arr_[0] = {.id = 0,
                        .name = "Air",
                        .move_slow_multiplier = block_defaults_.move_slow_multiplier,
                        .emits_light = false};
  loaded_ = true;
};

void BlockDB::WriteBlockData() const {
  EASSERT_MSG(loaded_, "Cannot write block data without having loaded it first");
  json block_data_arr_json = json::array();
  for (size_t i = 1; i < block_data_arr_.size(); i++) {
    json block_data_json = json::object();
    const auto& block_data = block_data_arr_[i];
    const auto& block_mesh_data = block_mesh_data_[i];
    block_data_json["id"] = block_data.id;
    block_data_json["model"] = block_model_names_[i];
    block_data_json["name"] = block_data.name;
    auto properties = json::object();
    if (block_data.move_slow_multiplier != block_defaults_.move_slow_multiplier) {
      properties["move_slow_multiplier"] = block_data.move_slow_multiplier;
    }
    if (block_data.emits_light != block_defaults_.emits_light) {
      properties["emits_light"] = block_data.emits_light;
    }
    block_data_json["properties"] = properties;
    block_data_arr_json.emplace_back(block_data_json);
  }
  json_util::WriteJson(block_data_arr_json, GET_PATH("resources/data/test_block_data_write.json"));
}

void BlockDB::LoadDefaultData(std::unordered_map<std::string, uint32_t>& name_to_idx) {
  ZoneScoped;
  json default_block_data = util::LoadJsonFile(GET_PATH("resources/data/block/default_block.json"));
  block_defaults_.name = default_block_data["name"].get<std::string>();
  block_defaults_.model = default_block_data["model"].get<std::string>();
  auto default_mesh_data = LoadBlockModel("block/default", name_to_idx);
  EASSERT_MSG(default_mesh_data.has_value(), "Default mesh data failed to load");
  default_mesh_data_ = default_mesh_data.value();

  auto& properties = default_block_data["properties"];
  block_defaults_.move_slow_multiplier = properties["move_slow_multiplier"].get<float>();
  block_defaults_.emits_light = properties["emits_light"].get<bool>();
}
void BlockDB::LoadAllBlockModels(std::unordered_map<std::string, uint32_t>& tex_name_to_idx) {
  EASSERT_MSG(block_defaults_loaded_, "Default block data must be loaded first");
  for (const auto& file :
       std::filesystem::directory_iterator(GET_PATH("resources/data/model/block"))) {
    std::string model_name = "block/" + file.path().stem().string();
    all_block_model_names_.emplace_back(model_name);
    auto block_mesh_data = LoadBlockModel(model_name, tex_name_to_idx);
    // model_name_to_mesh_data.emplace(model_name, block_mesh_data.value_or(default_mesh_data));
  }
}

std::optional<BlockMeshData> BlockDB::LoadBlockModel(
    const std::string& model_name, std::unordered_map<std::string, uint32_t>& tex_name_to_idx) {
  ZoneScoped;
  std::string relative_path = GET_PATH("resources/data/model/" + model_name + ".json");
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

  auto get_tex_idx = [this, &tex_obj, &tex_name_to_idx,
                      &relative_path](const std::string& type) -> uint32_t {
    auto tex_name = json_util::GetString(tex_obj.value(), type);
    if (tex_name.has_value()) {
      return tex_name_to_idx[tex_name.value()];
    }
    spdlog::error("model of type {} does not have required texture fields: {}", type,
                  relative_path);
    return default_mesh_data_.texture_indices[0];
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
    spdlog::error("Block model {} contains invalid type: {}", model_name, block_model_type.value());
    return std::nullopt;
  }
  return block_mesh_data;
}

void BlockDB::LoadBlockModelData(
    std::unordered_map<std::string, uint32_t>& tex_name_to_idx,
    std::unordered_map<std::string, BlockMeshData>& model_name_to_mesh_data) {
  ZoneScoped;
  EASSERT_MSG(block_defaults_loaded_, "Default block data must be loaded first");
  // Array of strings of block models to load
  json block_model_arr = util::LoadJsonFile(GET_PATH("resources/data/block/block_model_data.json"));
  // Load each block model file

  for (auto& item : block_model_arr) {
    std::string model_name = item.get<std::string>();
    auto block_mesh_data = LoadBlockModel(model_name, tex_name_to_idx);
    model_name_to_mesh_data.emplace(model_name, block_mesh_data.value_or(default_mesh_data_));
  }
}
