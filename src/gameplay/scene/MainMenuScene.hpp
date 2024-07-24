#pragma once

#include "application/Scene.hpp"

class WorldManager;

class MainMenuScene : public Scene {
 public:
  explicit MainMenuScene(SceneManager& scene_manager);
  void OnImGui() override;
  bool OnEvent(const SDL_Event& event) override;
  ~MainMenuScene() override;

 private:
  std::unique_ptr<WorldManager> world_manager_{nullptr};
};
