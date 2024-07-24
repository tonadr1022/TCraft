#pragma once

#include <array>
#include <nlohmann/json_fwd.hpp>

struct BlockMeshData {
  // pos x,neg x, pos y, neg y, pos z, neg z
  std::array<int, 6> texture_indices;
};

struct BlockData {
  std::string name;
  float move_slow_multiplier;
  bool emits_light;
};

class BlockDB {
 public:
  [[nodiscard]] const std::vector<BlockMeshData>& GetMeshData() const {
    return block_mesh_data_db_;
  };

  void Init(std::unordered_map<std::string, uint32_t>& name_to_idx);

 private:
  struct BlockDataDefaults {
    std::string name;
    std::string model;
    float move_slow_multiplier;
    bool emits_light;
    uint32_t model_tex_index;
  };
  BlockDataDefaults block_defaults_;

  std::vector<BlockData> block_data_db_;
  std::vector<BlockMeshData> block_mesh_data_db_;

  BlockMeshData default_mesh_data_;

  void LoadDefaultData(std::unordered_map<std::string, uint32_t>& name_to_idx);
  void LoadBlockModelData(std::unordered_map<std::string, uint32_t>& name_to_idx,
                          std::unordered_map<std::string, BlockMeshData>& model_name_to_mesh_data);
  std::optional<BlockMeshData> LoadBlockModel(
      const std::string& model_filename, std::unordered_map<std::string, uint32_t>& name_to_idx);
};
