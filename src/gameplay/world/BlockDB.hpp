#pragma once

#include <array>
#include <glm/vec3.hpp>
#include <nlohmann/json_fwd.hpp>

#include "gameplay/world/ChunkDef.hpp"

struct Image;

struct BlockMeshData {
  // pos x,neg x, pos y, neg y, pos z, neg z
  std::array<uint32_t, 6> texture_indices;
  std::array<glm::ivec3, 6> avg_colors;
};

struct BlockData {
  BlockType id;
  std::string full_file_path;
  std::string name;
  std::string formatted_name;
  float move_slow_multiplier;
  bool emits_light;
  bool operator==(const BlockData& other) const;
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
  BlockDB();
  [[nodiscard]] const std::vector<BlockMeshData>& GetMeshData() const;
  [[nodiscard]] const std::vector<BlockData>& GetBlockData() const;
  [[nodiscard]] const std::unordered_set<std::string>& GetTextureNamesInUse() const;
  static std::optional<BlockModelType> StringToBlockModelType(const std::string& model_type);

  // void Init();
  void LoadMeshData(std::unordered_map<std::string, uint32_t>& tex_name_to_idx);
  void LoadMeshData(std::unordered_map<std::string, uint32_t>& tex_name_to_idx,
                    const std::vector<Image>& image_data);
  void WriteBlockData(const BlockData& data, const std::string& model_name) const;
  static void WriteBlockModelTypeAll(const BlockModelDataAll& data, const std::string& path);
  static void WriteBlockModelTypeTopBot(const BlockModelDataTopBot& data, const std::string& path);
  static void WriteBlockModelTypeUnique(const BlockModelDataUnique& data, const std::string& path);
  void LoadBlockData();
  const BlockData* GetBlockData(const std::string& name) const;

  [[nodiscard]] static std::vector<std::string> GetAllBlockTexturesFromAllModels();
  [[nodiscard]] static std::vector<std::string> GetAllBlockModelNames();
  [[nodiscard]] static std::vector<std::string> GetAllTextureNames();

  [[nodiscard]] inline bool MeshDataInitialized() const { return mesh_data_initialized_; }

 private:
  // only the editor has full access to adding and changing data at runtime
  friend class BlockEditorScene;
  bool mesh_data_initialized_{false};

  std::vector<BlockData> block_data_arr_;
  std::vector<BlockMeshData> block_mesh_data_;
  std::vector<std::string> block_model_names_;

  static std::optional<BlockModelData> LoadBlockModelDataFromPath(const std::string& path);
  static std::optional<BlockModelData> LoadBlockModelDataFromName(const std::string& model_name);
  std::optional<BlockModelData> LoadBlockModelData(const std::string& model_name);

  struct BlockDataDefaults {
    uint32_t id;
    std::string name;
    std::string model_name;
    std::string tex_name;
    float move_slow_multiplier;
    bool emits_light;
  };
  BlockDataDefaults block_defaults_;
  BlockMeshData default_mesh_data_;
  bool block_defaults_loaded_{false};

  std::unordered_set<std::string> block_tex_names_in_use_;
  std::unordered_map<std::string, BlockModelData> model_name_to_model_data_;
  std::unordered_map<std::string, uint32_t> block_name_to_id_;
};
