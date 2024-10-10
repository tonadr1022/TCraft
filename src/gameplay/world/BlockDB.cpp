#include "BlockDB.hpp"

#include <filesystem>
#include <iostream>

#include "application/SettingsManager.hpp"
#include "gameplay/world/ChunkDef.hpp"
#include "renderer/RendererUtil.hpp"
#include "resource/Image.hpp"
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

void BlockDB::LoadMeshData(std::unordered_map<std::string, uint32_t>& tex_name_to_idx,
                           const std::vector<Image>& image_data) {
  mesh_data_initialized_ = true;
  block_mesh_data_.clear();
  // reserve 0 index for air
  block_mesh_data_.emplace_back(default_mesh_data_);

  auto get_avg_color = [](const Image& img) -> glm::ivec3 {
    glm::ivec3 pix_counts{0, 0, 0};
    for (int i = 0; i < img.width * img.height * 4; i += 4) {
      pix_counts.x += img.pixels[i];
      pix_counts.y += img.pixels[i + 1];
      pix_counts.z += img.pixels[i + 2];
    }
    pix_counts /= (img.width * img.height);
    return pix_counts;
  };

  // load block mesh data array
  for (size_t i = 1; i < block_model_names_.size(); i++) {
    const auto& model_name = block_model_names_[i];
    BlockModelData data_general = model_name_to_model_data_[model_name];
    BlockMeshData mesh_data = default_mesh_data_;
    if (BlockModelDataAll* data = std::get_if<BlockModelDataAll>(&data_general)) {
      auto idx = tex_name_to_idx[data->tex_all];
      std::fill(mesh_data.transparency_type.begin(), mesh_data.transparency_type.end(),
                static_cast<TransparencyType>(data->transparency_type));
      std::fill(mesh_data.texture_indices.begin(), mesh_data.texture_indices.end(), idx);
      std::fill(mesh_data.avg_colors.begin(), mesh_data.avg_colors.end(),
                get_avg_color(image_data[idx]));
    } else if (BlockModelDataTopBot* data = std::get_if<BlockModelDataTopBot>(&data_general)) {
      uint32_t side_idx = tex_name_to_idx[data->tex_side];
      uint32_t top_idx = tex_name_to_idx[data->tex_top];
      uint32_t bot_idx = tex_name_to_idx[data->tex_bottom];
      glm::ivec3 side_avg_color = get_avg_color(image_data[side_idx]);
      mesh_data.avg_colors = {side_avg_color,
                              side_avg_color,
                              get_avg_color(image_data[top_idx]),
                              get_avg_color(image_data[bot_idx]),
                              side_avg_color,
                              side_avg_color};
      mesh_data.texture_indices = {side_idx, side_idx, top_idx, bot_idx, side_idx, side_idx};
    } else if (BlockModelDataUnique* data = std::get_if<BlockModelDataUnique>(&data_general)) {
      uint32_t pos_x_idx = tex_name_to_idx[data->tex_pos_x],
               neg_x_idx = tex_name_to_idx[data->tex_neg_x],
               pos_y_idx = tex_name_to_idx[data->tex_pos_y],
               neg_y_idx = tex_name_to_idx[data->tex_neg_y],
               pos_z_idx = tex_name_to_idx[data->tex_pos_z],
               neg_z_idx = tex_name_to_idx[data->tex_neg_z];
      mesh_data.texture_indices = {
          pos_x_idx, neg_x_idx, pos_y_idx, neg_y_idx, pos_z_idx, neg_z_idx,
      };
      mesh_data.avg_colors = {
          get_avg_color(image_data[pos_x_idx]), get_avg_color(image_data[neg_x_idx]),
          get_avg_color(image_data[pos_y_idx]), get_avg_color(image_data[neg_y_idx]),
          get_avg_color(image_data[pos_z_idx]), get_avg_color(image_data[neg_z_idx]),
      };
    }
    block_mesh_data_.emplace_back(mesh_data);
  }
}

void BlockDB::LoadMeshData(std::unordered_map<std::string, uint32_t>& tex_name_to_idx) {
  mesh_data_initialized_ = true;
  // reserve 0 index for air
  block_mesh_data_.clear();
  block_mesh_data_.reserve(block_model_names_.size());
  block_mesh_data_.emplace_back(default_mesh_data_);

  // load block mesh data array
  for (size_t i = 1; i < block_model_names_.size(); i++) {
    const auto& model_name = block_model_names_[i];
    BlockModelData data_general = model_name_to_model_data_[model_name];
    BlockMeshData mesh_data = default_mesh_data_;
    if (BlockModelDataAll* data = std::get_if<BlockModelDataAll>(&data_general)) {
      std::fill(mesh_data.transparency_type.begin(), mesh_data.transparency_type.end(),
                static_cast<TransparencyType>(data->transparency_type));
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

const BlockData* BlockDB::GetBlockData(const std::string& name) const {
  auto it = block_name_to_id_.find(name);
  if (it == block_name_to_id_.end()) return nullptr;
  return &block_data_arr_[it->second];
}

BlockDB::BlockDB() {
  ZoneScoped;
  {
    // Default data load cannot fail.
    ZoneScopedN("Load default block data");
    json default_block_data =
        util::LoadJsonFile(GET_PATH("resources/data/block/default_block.json"));
    block_defaults_.name = default_block_data["name"].get<std::string>();
    block_defaults_.model_name = default_block_data["model"].get<std::string>();
    auto& properties = default_block_data["properties"];
    block_defaults_.move_slow_multiplier = properties["move_slow_multiplier"].get<float>();
    block_defaults_.emits_light = properties["emits_light"].get<bool>();
    block_defaults_.tex_name = "block/default";
    block_defaults_.id = default_block_data["id"].get<uint32_t>();
  }

  LoadBlockData();

  {
    ZoneScopedN("Load block model data");
    // cannot fail
    default_model_data_ = LoadBlockModelData("block/default").value();

    // Load each block model file (skip air)
    for (size_t i = 1; i < block_model_names_.size(); i++) {
      AddBlockModel(block_model_names_[i]);
    }
  }
};

void BlockDB::AddBlockModel(const std::string& model_name) {
  auto block_model_data = LoadBlockModelData(model_name);
  if (block_model_data.has_value()) {
    model_name_to_model_data_.emplace(model_name, block_model_data.value());
  } else {
    model_name_to_model_data_.emplace(model_name, default_model_data_);
  }
}

void BlockDB::LoadBlockData() {
  ZoneScoped;
  // ensure that multiple IDs cannot exist
  std::unordered_set<uint32_t> processed;

  // Ids are not guaranteed to be in order in the file-system, so load them into a map and then
  // populate the arrays after
  std::unordered_map<uint32_t, BlockData> block_id_to_data;
  std::unordered_map<uint32_t, std::string> block_id_to_model_name;
  for (const auto& file : std::filesystem::directory_iterator(GET_PATH("resources/data/block"))) {
    auto block_data = util::LoadJsonFile(file.path());
    BlockData data;
    data.name = file.path().filename().stem();
    data.formatted_name = block_data.value("name", block_defaults_.name);
    data.full_file_path = file.path();
    data.id = block_data["id"].get<uint32_t>();
    if (block_data.contains("properties")) {
      auto properties = block_data["properties"];
      data.move_slow_multiplier =
          properties.value("move_slow_multiplier", block_defaults_.move_slow_multiplier);
      data.emits_light = properties.value("emits_light", block_defaults_.emits_light);
    }
    std::string model_name = block_data.value("model", block_defaults_.model_name);

    if (processed.contains(data.id)) {
      spdlog::error("Data with id {} of name {} already exists.", data.id,
                    block_id_to_data[data.id].formatted_name);
      continue;
    }
    processed.insert(data.id);

    block_id_to_data[data.id] = data;
    block_id_to_model_name[data.id] = model_name;
  }

  block_data_arr_.clear();
  block_model_names_.clear();
  // both are array such that the ids all fall in range, otherwise the id is invalid if not in range
  block_data_arr_.resize(processed.size() + 1);
  block_model_names_.resize(processed.size() + 1);
  // reserve air
  block_data_arr_[0] = {.id = 0,
                        .full_file_path = "",
                        .name = "air",
                        .formatted_name = "Air",
                        .move_slow_multiplier = block_defaults_.move_slow_multiplier,
                        .emits_light = false};
  block_name_to_id_.emplace(block_data_arr_[0].name, 0);
  block_model_names_[0] = "air";

  for (size_t i = 1; i < block_data_arr_.size(); i++) {
    if (!block_id_to_data.contains(i) || !block_id_to_model_name.contains(i)) {
      spdlog::info("Error: ID {} not found, invalid ID state", i);
      std::vector<uint32_t> ids(processed.begin(), processed.end());
      std::sort(ids.begin(), ids.end());
      int c = 0;
      for (int id : ids) {
        std::cout << id << ' ';
        if (c++ == 10) {
          std::cout << '\n';
          c = 0;
        }
      }
      std::cout << '\n';
      continue;
    }

    block_data_arr_[i] = block_id_to_data.at(i);
    block_model_names_[i] = block_id_to_model_name.at(i);
    block_name_to_id_.emplace(block_data_arr_[i].name, i);
  }
}

void BlockDB::WriteBlockData(const BlockData& block_data, const std::string& model_name) const {
  EASSERT_MSG(block_data.full_file_path != "", "Invalid file path");
  json properties = {
      {"move_slow_multiplier", block_data.move_slow_multiplier},
      {"emits_light", block_data.emits_light},
  };
  if (block_data.id == 0) {
    spdlog::error("Cannot write data, invalid block id {}", block_data.id);
  }
  json block_data_json = {{"id", block_data.id},
                          {"model", model_name},
                          {"name", block_data.formatted_name},
                          {"properties", properties}};
  util::json::WriteJson(block_data_json, block_data.full_file_path);
}

std::optional<BlockModelData> BlockDB::LoadBlockModelData(const std::string& model_name) {
  ZoneScoped;
  std::string path = GET_PATH("resources/data/model/" + model_name + ".json");
  json model_obj = util::LoadJsonFile(path);
  auto block_model_type = util::json::GetString(model_obj, "type");
  if (!block_model_type.has_value()) {
    return std::nullopt;
  }

  auto tex_obj = util::json::GetObject(model_obj, "textures");
  if (!tex_obj.has_value()) {
    return std::nullopt;
  }
  if (!tex_obj.value().is_object()) {
    spdlog::error("textures key of model must be an object: {}", path);
  }

  auto get_tex_name = [this, &tex_obj, &path](const std::string& type) -> std::string {
    auto tex_name = util::json::GetString(tex_obj.value(), type);
    if (tex_name.has_value()) {
      block_tex_names_in_use_.insert(tex_name.value());
      return tex_name.value();
    }
    spdlog::error("model of type {} does not have required texture fields: {}", type, path);
    return block_defaults_.model_name;
  };

  BlockModelData block_model_data;
  if (block_model_type == "block/all") {
    block_model_data = BlockModelDataAll{.tex_all = get_tex_name("all"),
                                         .transparency_type = static_cast<TransparencyType>(
                                             tex_obj.value().value("transparency_type", 0))};
  } else if (block_model_type == "block/top_bottom") {
    block_model_data = BlockModelDataTopBot{.tex_top = get_tex_name("top"),
                                            .tex_bottom = get_tex_name("bottom"),
                                            .tex_side = get_tex_name("side")};
  } else if (block_model_type == "block/unique") {
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
    auto tex_obj = util::json::GetObject(model_obj, "textures");
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
  static const std::unordered_map<std::string, BlockModelType> kModelTypeMap = {
      {"block/all", BlockModelType::kAll},
      {"block/top_bottom", BlockModelType::kTopBottom},
      {"block/unique", BlockModelType::kUnique},
  };
  auto it = kModelTypeMap.find(model_type);
  if (it == kModelTypeMap.end()) return std::nullopt;
  return it->second;
}

std::optional<BlockModelData> BlockDB::LoadBlockModelDataFromPath(const std::string& path) {
  json model_obj = util::LoadJsonFile(path);
  auto tex_obj = util::json::GetObject(model_obj, "textures");
  auto type_str = util::json::GetString(model_obj, "type");
  if (!tex_obj.has_value() || !tex_obj.value().is_object() || !type_str.has_value()) {
    return std::nullopt;
  }
  auto type = StringToBlockModelType(type_str.value());
  if (!type.has_value()) {
    return std::nullopt;
  }

  if (type == BlockModelType::kAll) {
    auto tex_name = util::json::GetString(tex_obj.value(), "all");
    if (!tex_name.has_value()) {
      return std::nullopt;
    }
    return BlockModelDataAll{tex_name.value(), static_cast<TransparencyType>(
                                                   tex_obj.value().value("transparency_type", 0))};
  }

  if (type == BlockModelType::kTopBottom) {
    auto tex_name_top = util::json::GetString(tex_obj.value(), "top");
    auto tex_name_bot = util::json::GetString(tex_obj.value(), "bottom");
    auto tex_name_side = util::json::GetString(tex_obj.value(), "side");
    if (!tex_name_bot.has_value() || !tex_name_top.has_value() || !tex_name_side.has_value()) {
      return std::nullopt;
    }
    return BlockModelDataTopBot{tex_name_top.value(), tex_name_bot.value(), tex_name_side.value()};
  }

  auto tex_name_posx = util::json::GetString(tex_obj.value(), "posx");
  auto tex_name_negx = util::json::GetString(tex_obj.value(), "negx");
  auto tex_name_posy = util::json::GetString(tex_obj.value(), "posy");
  auto tex_name_negy = util::json::GetString(tex_obj.value(), "negy");
  auto tex_name_posz = util::json::GetString(tex_obj.value(), "posz");
  auto tex_name_negz = util::json::GetString(tex_obj.value(), "negz");
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

std::vector<std::string> BlockDB::GetAllBlockModelNames() {
  std::vector<std::string> ret;
  for (const auto& file :
       std::filesystem::directory_iterator(GET_PATH("resources/data/model/block"))) {
    ret.emplace_back(file.path().stem().string());
  }
  return ret;
}

bool BlockData::operator==(const BlockData& other) const {
  return id == other.id && full_file_path == other.full_file_path && name == other.name &&
         formatted_name == other.formatted_name &&
         move_slow_multiplier == other.move_slow_multiplier && emits_light == other.emits_light;
}

void BlockDB::WriteBlockModelTypeAll(const BlockModelDataAll& data, const std::string& path) {
  TransparencyType type = util::renderer::LoadImageAndCheckHasTransparency(
      GET_PATH("resources/textures/") + data.tex_all + ".png");
  nlohmann::json j = {
      {"type", "block/all"},
      {"textures", {{"all", data.tex_all}, {"transparency_type", static_cast<int>(type)}}}};
  util::json::WriteJson(j, path);
}

void BlockDB::WriteBlockModelTypeTopBot(const BlockModelDataTopBot& data, const std::string& path) {
  nlohmann::json j = {
      {"type", "block/top_bottom"},
      {"textures", {{"top", data.tex_top}, {"bottom", data.tex_bottom}, {"side", data.tex_side}}}};
  util::json::WriteJson(j, path);
}

void BlockDB::WriteBlockModelTypeUnique(const BlockModelDataUnique& data, const std::string& path) {
  nlohmann::json j = {{"type", "block/unique"},
                      {
                          "textures",
                          {"posx", data.tex_pos_x},
                          {"negx", data.tex_neg_x},
                          {"posy", data.tex_pos_y},
                          {"negy", data.tex_neg_y},
                          {"posz", data.tex_pos_z},
                          {"negz", data.tex_neg_z},
                      }};
  util::json::WriteJson(j, path);
}
