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

  // TODO: refactor
  std::vector<SingleBlock> blocks_;

  std::vector<BlockData> block_data_;
  std::vector<BlockMeshData> block_mesh_data_;
  std::unordered_map<std::string, uint32_t> tex_name_to_idx_;

  std::array<SingleBlock, 3> add_model_blocks_;

 private:
  void Reload();
  void HandleAddModelTextureChange();
  bool add_model_editor_open_{false};
  uint32_t add_model_tex_type_idx_{0};

  BlockModelDataAll edit_model_type_all_;
  BlockModelDataTopBot edit_model_type_top_bot_;
  BlockModelDataUnique edit_model_type_unique_;
};
