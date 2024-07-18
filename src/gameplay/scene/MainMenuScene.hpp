#pragma once

#include "application/Scene.hpp"

class MainMenuScene : public Scene {
 public:
  explicit MainMenuScene(SceneManager& scene_manager);
  void Init() override;
  void Update(double dt) override;
  void OnImGui() override;
  bool OnEvent(const SDL_Event& event) override;
  void Render(Renderer& renderer, const Window& window) override;
};
