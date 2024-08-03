#pragma once

#include <SDL_events.h>

#include "camera/FPSCamera.hpp"
#include "camera/OrbitCamera.hpp"

class Player {
 public:
  enum class CameraMode { FPS, Orbit };
  void Init() const;
  void Update(double dt);
  void OnImGui() const;
  bool OnEvent(const SDL_Event& event);
  void SetCameraFocused(bool state);
  void SetPosition(const glm::vec3& pos);
  [[nodiscard]] const glm::vec3& Position() const;
  Camera& GetCamera();
  CameraMode camera_mode{CameraMode::FPS};
  void LookAt(const glm::vec3& pos);

 private:
  glm::vec3 position_{0};
  FPSCamera fps_camera_;
  OrbitCamera orbit_camera_;
  bool camera_focused_{false};
  float move_speed_{0.01};
};
