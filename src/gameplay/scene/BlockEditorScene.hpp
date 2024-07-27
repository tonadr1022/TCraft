#pragma once

#include "application/Scene.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "renderer/ChunkMesh.hpp"

struct SingleBlock {
  ChunkMesh mesh;
  glm::vec3 pos;
  BlockType block;
};

class BlockEditorScene : public Scene {
 public:
  explicit BlockEditorScene(SceneManager& scene_manager);
  ~BlockEditorScene() override;

  void OnImGui() override;
  void Update(double dt) override;
  bool OnEvent(const SDL_Event& event) override;
  void Render(Renderer& renderer, const Window& window) override;

  struct RenderParams {
    uint32_t chunk_tex_array_handle{0};
  };

  BlockDB block_db_;

  RenderParams render_params_;
  Player player_;

  std::vector<BlockData> block_data_;
  std::vector<BlockMeshData> block_mesh_data_;
  std::vector<std::string> all_block_model_names_;
  std::unordered_set<std::string> all_block_model_names_set_;
  std::unordered_map<std::string, uint32_t> tex_name_to_idx_;

  std::array<SingleBlock, 3> add_model_blocks_;

 private:
  float block_rot_{0};
  void Reload();
  void HandleAddModelTextureChange(BlockModelType type);
  void HandleEditModelChange(BlockModelType type);
  void ResetAddModelData();
  bool add_model_editor_open_{false};

  BlockModelType add_model_type_{BlockModelType::All};
  BlockModelDataAll add_model_data_all_;
  BlockModelDataTopBot add_model_data_top_bot_;
  BlockModelDataUnique add_model_data_unique_;
  BlockModelType edit_model_type_{BlockModelType::All};
  BlockModelDataAll edit_model_data_all_;
  BlockModelDataTopBot edit_model_data_top_bot_;
  BlockModelDataUnique edit_model_data_unique_;
};
