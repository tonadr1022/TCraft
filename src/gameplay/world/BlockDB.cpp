#include "BlockDB.hpp"

#include <filesystem>

#include "application/SettingsManager.hpp"
#include "util/JsonUtil.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

using std::vector;

using json = nlohmann::json;

const std::vector<BlockMeshData>& BlockDB::GetMeshData() const { return block_mesh_data_; };
const std::vector<BlockData>& BlockDB::GetBlockData() const { return block_data_arr_; };

void BlockDB::Init(std::unordered_map<std::string, uint32_t>& tex_name_to_idx,
                   bool load_all_block_model_data, bool load_all_texture_names) {
  ZoneScoped;
  std::unordered_map<std::string, BlockMeshData> model_name_to_mesh_data;
  {
    ZoneScopedN("Load default block data");
    json default_block_data =
        util::LoadJsonFile(GET_PATH("resources/data/block/default_block.json"));
    block_defaults_.name = default_block_data["name"].get<std::string>();
    block_defaults_.model = default_block_data["model"].get<std::string>();
    // TODO: refactor block model loading
    auto block_model_data = LoadBlockModelDataFromName("block/default");
    block_defaults_.tex_name = std::get<BlockModelDataAll>(block_model_data.value()).tex_all;
    spdlog::info("{}", block_defaults_.tex_name);
    auto default_mesh_data = LoadBlockModel("block/default", tex_name_to_idx);
    EASSERT_MSG(default_mesh_data.has_value(), "Default mesh data failed to load");
    default_mesh_data_ = default_mesh_data.value();

    auto& properties = default_block_data["properties"];
    block_defaults_.move_slow_multiplier = properties["move_slow_multiplier"].get<float>();
    block_defaults_.emits_light = properties["emits_light"].get<bool>();
  }

  if (load_all_block_model_data) {
    ZoneScopedN("Load all block model data");
    for (const auto& file :
         std::filesystem::directory_iterator(GET_PATH("resources/data/model/block"))) {
      std::string model_name = "block/" + file.path().stem().string();
      auto block_mesh_data = LoadBlockModel(model_name, tex_name_to_idx);
      model_name_to_mesh_data.emplace(model_name, block_mesh_data.value_or(default_mesh_data_));
    }
  } else {
    ZoneScopedN("Load block model data");
    // Array of strings of block models to load
    json block_model_arr =
        util::LoadJsonFile(GET_PATH("resources/data/block/block_model_data.json"));
    // Load each block model file
    for (auto& item : block_model_arr) {
      std::string model_name = item.get<std::string>();
      auto block_mesh_data = LoadBlockModel(model_name, tex_name_to_idx);
      model_name_to_mesh_data.emplace(model_name, block_mesh_data.value_or(default_mesh_data_));
    }
  }

  {
    ZoneScopedN("Load block data");
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
  }

  if (load_all_texture_names) ReloadTextureNames();
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

std::optional<BlockMeshData> BlockDB::LoadBlockModel(
    const std::string& model_name, std::unordered_map<std::string, uint32_t>& tex_name_to_idx) {
  ZoneScoped;
  std::string path = GET_PATH("resources/data/model/" + model_name + ".json");
  json model_obj = util::LoadJsonFile(path);
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
    spdlog::error("textures key of model must be an object: {}", path);
  }
  auto get_tex_idx = [this, &tex_obj, &tex_name_to_idx,
                      &path](const std::string& type) -> uint32_t {
    auto tex_name = json_util::GetString(tex_obj.value(), type);
    if (tex_name.has_value()) {
      return tex_name_to_idx[tex_name.value()];
    }
    spdlog::error("model of type {} does not have required texture fields: {}", type, path);
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

std::vector<std::string> BlockDB::GetAllBlockTexturesFromAllModels() {
  std::unordered_set<std::string> tex_names_set;
  for (const auto& file :
       std::filesystem::directory_iterator(GET_PATH("resources/data/model/block"))) {
    json model_obj = util::LoadJsonFile(file.path().string());
    auto tex_obj = json_util::GetObject(model_obj, "textures");
    if (!tex_obj.has_value() || !tex_obj.value().is_object()) {
      spdlog::error("Invalid model format: {}", file.path().string());
      continue;
    }
    for (auto it = tex_obj->begin(); it != tex_obj->end(); it++) {
      tex_names_set.insert(it.value());
    }
  }

  return {tex_names_set.begin(), tex_names_set.end()};
}

void BlockDB::ReloadTextureNames() {
  all_block_texture_names_.clear();
  for (const auto& file :
       std::filesystem::directory_iterator(GET_PATH("resources/textures/block"))) {
    all_block_texture_names_.emplace_back("block/" + file.path().stem().string());
  }
  std::sort(all_block_texture_names_.begin(), all_block_texture_names_.end());
}

std::optional<BlockModelType> BlockDB::StringToBlockModelType(const std::string& model_type) {
  static const std::unordered_map<std::string, BlockModelType> ModelTypeMap = {
      {"block/all", BlockModelType::All},
      {"block/top_bottom", BlockModelType::TopBottom},
      {"block/unique", BlockModelType::Unique},
  };
  auto it = ModelTypeMap.find(model_type);
  if (it == ModelTypeMap.end()) return std::nullopt;
  return it->second;
}

std::optional<BlockModelData> BlockDB::LoadBlockModelDataFromPath(const std::string& path) {
  json model_obj = util::LoadJsonFile(path);
  auto tex_obj = json_util::GetObject(model_obj, "textures");
  auto type_str = json_util::GetString(model_obj, "type");
  if (!tex_obj.has_value() || !tex_obj.value().is_object() || !type_str.has_value()) {
    return std::nullopt;
  }
  auto type = StringToBlockModelType(type_str.value());
  if (!type.has_value()) {
    return std::nullopt;
  }

  if (type == BlockModelType::All) {
    auto tex_name = json_util::GetString(tex_obj.value(), "all");
    if (!tex_name.has_value()) {
      return std::nullopt;
    }
    return BlockModelDataAll{tex_name.value()};
  }

  if (type == BlockModelType::TopBottom) {
    auto tex_name_top = json_util::GetString(tex_obj.value(), "top");
    auto tex_name_bot = json_util::GetString(tex_obj.value(), "bottom");
    if (!tex_name_bot.has_value() || !tex_name_top.has_value()) {
      return std::nullopt;
    }
    return BlockModelDataTopBot{tex_name_top.value(), tex_name_bot.value()};
  }

  auto tex_name_posx = json_util::GetString(tex_obj.value(), "posx");
  auto tex_name_negx = json_util::GetString(tex_obj.value(), "negx");
  auto tex_name_posy = json_util::GetString(tex_obj.value(), "posy");
  auto tex_name_negy = json_util::GetString(tex_obj.value(), "negy");
  auto tex_name_posz = json_util::GetString(tex_obj.value(), "posz");
  auto tex_name_negz = json_util::GetString(tex_obj.value(), "negz");
  if (!tex_name_posx.has_value() || !tex_name_negx.has_value() || !tex_name_posy.has_value() ||
      !tex_name_negy.has_value() || !tex_name_posz.has_value() || !tex_name_negz.has_value()) {
    return std::nullopt;
  }
  return BlockModelDataUnique{tex_name_posx.value(), tex_name_negx.value(), tex_name_posy.value(),
                              tex_name_negy.value(), tex_name_posz.value(), tex_name_negz.value()};
}

std::optional<BlockModelData> BlockDB::LoadBlockModelDataFromName(const std::string& model_name) {
  return LoadBlockModelDataFromPath(GET_PATH("resources/data/model/" + model_name + ".json"));
}

std::vector<std::string> BlockDB::GetAllModelNames() {
  std::vector<std::string> ret;
  for (const auto& file :
       std::filesystem::directory_iterator(GET_PATH("resources/data/model/block"))) {
    ret.emplace_back("block/" + file.path().stem().string());
  }
  return ret;
}
