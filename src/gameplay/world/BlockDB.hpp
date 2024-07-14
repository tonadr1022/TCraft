#pragma once

struct BlockMeshData {};

struct BlockData {};

class BlockDB {
 public:
  static BlockDB& Get();

 private:
  friend class Application;
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
};
