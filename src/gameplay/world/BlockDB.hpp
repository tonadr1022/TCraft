#pragma once

#include <array>
struct BlockMeshData {
  // pos x,neg x, pos y, neg y, pos z, neg z
  std::array<int, 6> texture_indices;
};

struct BlockData {};

class BlockDB {
 public:
  static BlockDB& Get();

 private:
  friend class Application;
  friend class ResourceLoader;
  void LoadData();
  BlockDB();
  static BlockDB* instance_;

  struct BlockDataDefaults {
    std::string name;
    std::string model;
    float move_slow_multiplier;
    bool emits_light;
  };
  BlockDataDefaults block_defaults_;

  std::vector<BlockData> block_data_db_;
  std::vector<BlockMeshData> block_mesh_data_db_;

  void LoadDefaultData();
  void LoadBlockModelData(std::unordered_map<std::string, BlockMeshData>& mesh_data_map);
};
