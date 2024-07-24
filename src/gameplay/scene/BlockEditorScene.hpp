#pragma once

#include "application/Scene.hpp"

class BlockEditorScene : public Scene {
 public:
  explicit BlockEditorScene(SceneManager& scene_manager);
  void OnImGui() override;
  bool OnEvent(const SDL_Event& event) override;
  ~BlockEditorScene() override;
};
