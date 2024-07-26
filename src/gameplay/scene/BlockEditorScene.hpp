#pragma once

#include "application/Scene.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "renderer/ChunkMesh.hpp"
#include "renderer/FPSCamera.hpp"

struct SingleBlock {
  ChunkMesh mesh;
  glm::ivec3 pos;
  BlockType block;
};

class BlockEditorScene : public Scene {
 public:
  explicit BlockEditorScene(SceneManager& scene_manager);
  ~BlockEditorScene() override;

  void OnImGui() override;
  bool OnEvent(const SDL_Event& event) override;
  void Render(Renderer& renderer, const Window& window) override;

  struct RenderParams {
    uint32_t chunk_tex_array_handle{0};
  };

  BlockDB block_db_;

  RenderParams render_params_;
  FPSCamera fps_camera_;

 private:
  bool add_model_editor_open_{false};
  std::vector<SingleBlock> blocks_;
  std::array<int, 6> add_model_tex_indexes_;
};
