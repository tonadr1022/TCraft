#pragma once

#include <SDL_events.h>

#include "camera/Camera.hpp"

class OrbitCamera : public Camera {
 public:
  [[nodiscard]] glm::mat4 GetProjection(float aspect_ratio) const override;
  [[nodiscard]] glm::mat4 GetView() const override;
  void SetPosition(const glm::vec3& pos) override;
  bool OnEvent(const SDL_Event& event) override;
  void Update(double dt) override;
  void OnImGui() const override;
  void LookAt(const glm::vec3& pos) override;

 private:
  glm::vec3 target_{0, 0, 0};
  glm::vec3 up_;
  glm::vec3 right_;
  glm::vec3 pos_;
  glm::vec3 front_;

  float distance_;
  float azimuth_angle_;
  float polar_angle_;

  constexpr inline static glm::vec3 kUpVector = {0, 1, 0};
  static constexpr const float kMinDistance = 0.5f;
  static constexpr const float kDefaultOrbitScrollSensitivity = 0.25f;
  bool first_mouse_{true};
  void UpdatePosition();
};
