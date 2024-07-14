#pragma once

#include <SDL_events.h>

#include "renderer/FPSCamera.hpp"

class Player {
 public:
  void Init() const;
  void Update(double dt);
  FPSCamera& GetCamera() { return fps_camera_; }
  void OnImGui() const;
  bool OnEvent(const SDL_Event& event);

 private:
  FPSCamera fps_camera_;
  bool camera_focused_{true};
  float move_speed_{0.01};
  glm::vec3 position_{0};
};
