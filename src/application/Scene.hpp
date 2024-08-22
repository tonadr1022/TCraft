#pragma once

#include <SDL_events.h>

#include "renderer/RenderInfo.hpp"
#include "ui/UI.hpp"

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
  RenderInfo curr_render_info_;
  ui::UI ui_;
};
