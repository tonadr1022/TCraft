#pragma once

#include "application/Scene.hpp"
#include "renderer/FPSCamera.hpp"

class BlockEditorScene : public Scene {
 public:
  explicit BlockEditorScene(SceneManager& scene_manager);
  void OnImGui() override;
  bool OnEvent(const SDL_Event& event) override;
  void Render(Renderer& renderer, const Window& window) override;
  ~BlockEditorScene() override;
  FPSCamera fps_camera_;
};
