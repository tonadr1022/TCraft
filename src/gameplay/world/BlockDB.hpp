#pragma once

#include <array>
#include <nlohmann/json_fwd.hpp>

#include "gameplay/world/Block.hpp"

struct BlockMeshData {
  // pos x,neg x, pos y, neg y, pos z, neg z
  std::array<int, 6> texture_indices;
};

struct BlockData {
  BlockType id;
  std::string name;
  float move_slow_multiplier;
  bool emits_light;
};

class BlockDB {
 public:
  [[nodiscard]] const std::vector<BlockMeshData>& GetMeshData() const;
  [[nodiscard]] const std::vector<BlockData>& GetBlockData() const;

  void Init(std::unordered_map<std::string, uint32_t>& name_to_idx);
  void WriteBlockData() const;
  void LoadAllBlockModelNames();

 private:
  // only the editor has full access to adding and changing data at runtime
  friend class BlockEditorScene;
  struct BlockDataDefaults {
    std::string name;
    std::string model;
    float move_slow_multiplier;
    bool emits_light;
    uint32_t model_tex_index;
  };
  BlockDataDefaults block_defaults_;

  std::vector<BlockData> block_data_arr_;
  std::vector<BlockMeshData> block_mesh_data_;
  std::vector<std::string> block_model_names_;

  std::vector<std::string> all_block_model_names_;

  BlockMeshData default_mesh_data_;

  void LoadDefaultData(std::unordered_map<std::string, uint32_t>& name_to_idx);
  void LoadBlockModelData(std::unordered_map<std::string, uint32_t>& name_to_idx,
                          std::unordered_map<std::string, BlockMeshData>& model_name_to_mesh_data);
  std::optional<BlockMeshData> LoadBlockModel(
      const std::string& model_name, std::unordered_map<std::string, uint32_t>& name_to_idx);

  bool loaded_{false};
};
