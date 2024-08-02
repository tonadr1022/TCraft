#pragma once

#include <SDL_events.h>

class SceneManager;
class Window;

class Scene {
 public:
  explicit Scene(SceneManager& scene_manager);
  virtual ~Scene();
  virtual void Update(double dt);
  virtual void OnImGui();
  virtual void Render();
  virtual bool OnEvent(const SDL_Event& event);

 protected:
  SceneManager& scene_manager_;
  Window& window_;
};
