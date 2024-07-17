#pragma once

#include <SDL_events.h>

class Scene {
 public:
  Scene() = default;
  virtual void Init() = 0;
  virtual void Update(double dt) = 0;
  virtual void OnImGui() const = 0;
  virtual bool OnEvent(const SDL_Event& event) = 0;
};
