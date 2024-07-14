#pragma once

#include <SDL_events.h>

#include "renderer/FPSCamera.hpp"

class Player {
 public:
  void Init();
  void Update(double dt);
  FPSCamera& GetCamera() { return fps_camera_; }
  void OnImGui() const;
  bool OnEvent(const SDL_Event& event);
  void OnCameraFocusChange();

  glm::vec3 position_{0};

 private:
  FPSCamera fps_camera_;
  bool camera_focused_{false};
  float move_speed_{0.01};
};
