#pragma once

#include <SDL_events.h>

class SceneManager;
class Window;
class Renderer;

class Scene {
 public:
  explicit Scene(SceneManager& scene_manager) : scene_manager_(scene_manager) {}
  virtual ~Scene() = default;
  virtual void Update(double dt);
  virtual void OnImGui();
  virtual void Render(Renderer& renderer, const Window& window);
  virtual bool OnEvent(const SDL_Event& event);
  std::string name_;

 protected:
  SceneManager& scene_manager_;
};
