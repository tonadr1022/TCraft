#pragma once

#include <array>
#include <nlohmann/json_fwd.hpp>

#include "gameplay/world/Block.hpp"

struct BlockMeshData {
  // pos x,neg x, pos y, neg y, pos z, neg z
  std::array<uint32_t, 6> texture_indices;
};

struct BlockData {
  BlockType id;
  std::string name;
  float move_slow_multiplier;
  bool emits_light;
};

enum class BlockModelType { All, TopBottom, Unique };

struct BlockModelDataAll {
  static const BlockModelType Type{BlockModelType::All};
  std::string tex_all;
};

struct BlockModelDataTopBot {
  static const BlockModelType Type{BlockModelType::TopBottom};
  std::string tex_top;
  std::string tex_bottom;
  std::string tex_side;
};

struct BlockModelDataUnique {
  static const BlockModelType Type{BlockModelType::Unique};
  std::string tex_pos_x;
  std::string tex_neg_x;
  std::string tex_pos_y;
  std::string tex_neg_y;
  std::string tex_pos_z;
  std::string tex_neg_z;
};

using BlockModelData = std::variant<BlockModelDataAll, BlockModelDataTopBot, BlockModelDataUnique>;

class BlockDB {
 public:
  [[nodiscard]] const std::vector<BlockMeshData>& GetMeshData() const;
  [[nodiscard]] const std::vector<BlockData>& GetBlockData() const;
  [[nodiscard]] const std::vector<std::string>& GetAllBlockTextureNames() const;
  static std::optional<BlockModelType> StringToBlockModelType(const std::string& model_type);

  void Init(std::unordered_map<std::string, uint32_t>& tex_name_to_idx,
            bool load_all_block_model_data, bool load_all_texture_names = false);
  void WriteBlockData() const;
  void ReloadTextureNames();

  static std::optional<BlockModelData> LoadBlockModelDataFromPath(const std::string& path);
  static std::optional<BlockModelData> LoadBlockModelDataFromName(const std::string& model_name);

  [[nodiscard]] static std::vector<std::string> GetAllBlockTexturesFromAllModels();
  [[nodiscard]] static std::vector<std::string> GetAllModelNames();

 private:
  // only the editor has full access to adding and changing data at runtime
  friend class BlockEditorScene;

  std::vector<BlockData> block_data_arr_;
  std::vector<BlockMeshData> block_mesh_data_;
  std::vector<std::string> block_model_names_;
  std::vector<std::string> all_block_texture_names_;

  std::optional<BlockMeshData> LoadBlockModel(
      const std::string& model_name, std::unordered_map<std::string, uint32_t>& name_to_idx);

  bool loaded_{false};

  struct BlockDataDefaults {
    std::string name;
    std::string model;
    std::string tex_name;
    float move_slow_multiplier;
    bool emits_light;
  };
  BlockDataDefaults block_defaults_;
  BlockMeshData default_mesh_data_;
  bool block_defaults_loaded_{false};
};
