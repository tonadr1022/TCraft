#pragma once

#include <SDL_events.h>

class SceneManager;
class Window;
class Renderer;

class Scene {
 public:
  explicit Scene(SceneManager& scene_manager) : scene_manager_(scene_manager) {}
  virtual ~Scene() = default;
  virtual void Init() = 0;
  virtual void Update(double dt) = 0;
  virtual void OnImGui() = 0;
  virtual void Render(Renderer& renderer, const Window& window) = 0;
  virtual bool OnEvent(const SDL_Event& event) = 0;

 protected:
  SceneManager& scene_manager_;
};
