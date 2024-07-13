#pragma once

#include "renderer/FPSCamera.hpp"
class Player {
 public:
  void Update(double dt);

 private:
  FPSCamera fps_camera_;
  float move_speed_{1};
  glm::vec3 position_{};
};
