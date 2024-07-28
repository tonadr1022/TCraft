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

// load block data that includes the models that are in use.
// Load each model, and grab each of the texture names they use and store temporarily
// with the textures in use, load the texture 2d array, and then update the mesh data

void BlockDB::LoadMeshData(std::unordered_map<std::string, uint32_t>& tex_name_to_idx) {
  // reserve 0 index for air
  block_mesh_data_.emplace_back(default_mesh_data_);

  // load block mesh data array
  for (int i = 1; i < block_model_names_.size(); i++) {
    const auto& model_name = block_model_names_[i];
    BlockModelData data_general = model_name_to_model_data_[model_name];
    BlockMeshData mesh_data = default_mesh_data_;
    if (BlockModelDataAll* data = std::get_if<BlockModelDataAll>(&data_general)) {
      std::fill(mesh_data.texture_indices.begin(), mesh_data.texture_indices.end(),
                tex_name_to_idx[data->tex_all]);
    } else if (BlockModelDataTopBot* data = std::get_if<BlockModelDataTopBot>(&data_general)) {
      uint32_t side_idx = tex_name_to_idx[data->tex_side];
      mesh_data.texture_indices = {
          side_idx, side_idx, tex_name_to_idx[data->tex_top], tex_name_to_idx[data->tex_bottom],
          side_idx, side_idx};
    } else if (BlockModelDataUnique* data = std::get_if<BlockModelDataUnique>(&data_general)) {
      mesh_data.texture_indices = {
          tex_name_to_idx[data->tex_pos_x], tex_name_to_idx[data->tex_neg_x],
          tex_name_to_idx[data->tex_pos_y], tex_name_to_idx[data->tex_neg_y],
          tex_name_to_idx[data->tex_pos_z], tex_name_to_idx[data->tex_neg_z],
      };
    }
    block_mesh_data_.emplace_back(mesh_data);
  }
}

const std::unordered_set<std::string>& BlockDB::GetTextureNamesInUse() const {
  return block_tex_names_in_use_;
}

void BlockDB::Init() {
  ZoneScoped;

  {
    // Default data load cannot fail.
    ZoneScopedN("Load default block data");
    json default_block_data =
        util::LoadJsonFile(GET_PATH("resources/data/block/default_block.json"));
    block_defaults_.name = default_block_data["name"].get<std::string>();
    block_defaults_.model_name = default_block_data["model"].get<std::string>();
    block_defaults_.tex_name = default_block_data["tex_name"].get<std::string>();
    auto& properties = default_block_data["properties"];
    block_defaults_.move_slow_multiplier = properties["move_slow_multiplier"].get<float>();
    block_defaults_.emits_light = properties["emits_light"].get<bool>();
  }

  LoadBlockData();

  {
    ZoneScopedN("Load block model data");
    auto default_model_data = LoadBlockModelData("block/default");
    EASSERT_MSG(default_model_data.has_value(), "Default mesh data failed to load");

    // Load each block model file (skip air)
    for (int i = 1; i < block_model_names_.size(); i++) {
      const auto& model_name = block_model_names_[i];
      auto block_model_data = LoadBlockModelData(model_name);
      if (block_model_data.has_value()) {
        model_name_to_model_data_.emplace(model_name, block_model_data.value());
      } else {
        model_name_to_model_data_.emplace(model_name, default_model_data.value());
      }
    }
  }
};

void BlockDB::LoadBlockData() {
  ZoneScoped;
  // ensure that multiple IDs cannot exist
  std::unordered_set<uint32_t> processed;

  uint32_t id = 1;
  // Ids are not guaranteed to be in order in the file-system, so load them into a map and then
  // populate the arrays after
  std::unordered_map<uint32_t, BlockData> block_id_to_data;
  std::unordered_map<uint32_t, std::string> block_id_to_model_name;
  for (const auto& file : std::filesystem::directory_iterator(GET_PATH("resources/data/block"))) {
    auto block_data = util::LoadJsonFile(file.path());
    BlockData data;
    data.name = block_data.value("name", block_defaults_.name);
    data.id = id++;
    if (block_data.contains("properties")) {
      auto properties = block_data["properties"];
      data.move_slow_multiplier =
          properties.value("move_slow_multiplier", block_defaults_.move_slow_multiplier);
      data.emits_light = properties.value("emits_light", block_defaults_.emits_light);
    }
    std::string model_name = block_data.value("model", block_defaults_.model_name);
    if (processed.contains(data.id)) {
      spdlog::error("Data with id {} already processed.", data.id);
      continue;
    }
    processed.insert(data.id);

    block_id_to_data[data.id] = data;
    block_id_to_model_name[data.id] = model_name;
  }

  block_data_arr_.clear();
  block_model_names_.clear();
  // reserve air
  block_data_arr_.emplace_back(
      BlockData{.id = 0,
                .filename = "",
                .name = "Air",
                .move_slow_multiplier = block_defaults_.move_slow_multiplier,
                .emits_light = false});
  block_model_names_.emplace_back("");

  for (uint32_t i = 1; i < id; i++) {
    block_data_arr_.emplace_back(block_id_to_data[i]);
    block_model_names_.emplace_back(block_id_to_model_name[i]);
  }
}

void BlockDB::WriteBlockData() const {
  for (size_t i = 1; i < block_data_arr_.size(); i++) {
    json block_data_json = json::object();
    const auto& block_data = block_data_arr_[i];
    const auto& block_mesh_data = block_mesh_data_[i];
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
    json_util::WriteJson(block_data_json,
                         GET_PATH("resources/data/block" + block_data.filename + ".json"));
  }
}

std::optional<BlockModelData> BlockDB::LoadBlockModelData(const std::string& model_name) {
  ZoneScoped;
  std::string path = GET_PATH("resources/data/model/" + model_name + ".json");
  json model_obj = util::LoadJsonFile(path);
  auto block_model_type = json_util::GetString(model_obj, "type");
  if (!block_model_type.has_value()) {
    return std::nullopt;
  }

  auto tex_obj = json_util::GetObject(model_obj, "textures");
  if (!tex_obj.has_value()) {
    return std::nullopt;
  }
  if (!tex_obj.value().is_object()) {
    spdlog::error("textures key of model must be an object: {}", path);
  }

  auto get_tex_name = [this, &tex_obj, &path](const std::string& type) -> std::string {
    auto tex_name = json_util::GetString(tex_obj.value(), type);
    if (tex_name.has_value()) {
      block_tex_names_in_use_.insert(tex_name.value());
      return tex_name.value();
    }
    spdlog::error("model of type {} does not have required texture fields: {}", type, path);
    return block_defaults_.model_name;
  };

  BlockModelData block_model_data;
  if (block_model_type == "block/all") {
    block_model_data = BlockModelDataAll{.tex_all = get_tex_name("all")};
    // int tex_index = get_tex_name("all");
    // for (auto& val : block_mesh_data.texture_indices) {
    //   val = tex_index;
    // }
  } else if (block_model_type == "block/top_bottom") {
    block_model_data = BlockModelDataTopBot{.tex_top = get_tex_name("top"),
                                            .tex_bottom = get_tex_name("bottom"),
                                            .tex_side = get_tex_name("side")};
    // block_mesh_data.texture_indices[2] = get_tex_name("top");
    // block_mesh_data.texture_indices[3] = get_tex_name("bottom");
    // int side_tex_idx = get_tex_name("side");
    // block_mesh_data.texture_indices[0] = side_tex_idx;
    // block_mesh_data.texture_indices[1] = side_tex_idx;
    // block_mesh_data.texture_indices[4] = side_tex_idx;
    // block_mesh_data.texture_indices[5] = side_tex_idx;
  } else if (block_model_type == "block/unique") {
    static constexpr const std::array<const char*, 6> SideStrs = {"posx", "negx", "posy", "posz",
                                                                  "negz"};

    block_model_data = BlockModelDataUnique{
        .tex_pos_x = get_tex_name("posx"),
        .tex_neg_x = get_tex_name("negx"),
        .tex_pos_y = get_tex_name("posy"),
        .tex_neg_y = get_tex_name("negy"),
        .tex_pos_z = get_tex_name("posz"),
        .tex_neg_z = get_tex_name("negz"),
    };
  } else {
    spdlog::error("Block model {} contains invalid type: {}", model_name, block_model_type.value());
    return std::nullopt;
  }
  return block_model_data;
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

std::vector<std::string> BlockDB::GetAllTextureNames() {
  std::vector<std::string> tex_names;
  for (const auto& file :
       std::filesystem::directory_iterator(GET_PATH("resources/textures/block"))) {
    tex_names.emplace_back("block/" + file.path().stem().string());
  }
  return tex_names;
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
    auto tex_name_side = json_util::GetString(tex_obj.value(), "side");
    if (!tex_name_bot.has_value() || !tex_name_top.has_value() || !tex_name_side.has_value()) {
      return std::nullopt;
    }
    return BlockModelDataTopBot{tex_name_top.value(), tex_name_bot.value(), tex_name_side.value()};
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
