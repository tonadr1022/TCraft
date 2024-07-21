#pragma once

#include "application/Scene.hpp"

class WorldManager;

class MainMenuScene : public Scene {
 public:
  explicit MainMenuScene(SceneManager& scene_manager);
  void Update(double dt) override;
  void OnImGui() override;
  bool OnEvent(const SDL_Event& event) override;
  void Render(Renderer& renderer, const Window& window) override;
  ~MainMenuScene() override;

 private:
  void CreateWorld();
  std::unique_ptr<WorldManager> world_manager_{nullptr};
};
