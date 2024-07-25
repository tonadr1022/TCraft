#pragma once

#include "application/Scene.hpp"
#include "renderer/FPSCamera.hpp"

class BlockDB;

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

  std::unique_ptr<BlockDB> block_db_{nullptr};

  RenderParams render_params_;
  FPSCamera fps_camera_;
};
