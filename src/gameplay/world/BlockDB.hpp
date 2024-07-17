#pragma once

#include <array>
struct BlockMeshData {
  // pos x,neg x, pos y, neg y, pos z, neg z
  std::array<int, 6> texture_indices;
};

struct BlockData {};

class BlockDB {
 public:
  BlockDB();

 private:
  struct BlockDataDefaults {
    std::string name;
    std::string model;
    float move_slow_multiplier;
    bool emits_light;
  };
  BlockDataDefaults block_defaults_;

  std::vector<BlockData> block_data_db_;
  std::vector<BlockMeshData> block_mesh_data_db_;
  std::unordered_map<std::string, BlockMeshData> model_name_to_mesh_data_;

  void LoadDefaultData();
  void LoadBlockModelData();
};
