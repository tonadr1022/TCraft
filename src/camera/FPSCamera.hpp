#pragma once

#include "camera/Camera.hpp"

class FPSCamera : public Camera {
 public:
  [[nodiscard]] glm::mat4 GetProjection(float aspect_ratio) const override;
  [[nodiscard]] glm::mat4 GetView() const override;

  static constexpr const glm::vec3 kUpVector{0.f, 1.f, 0.f};
  void Update(double dt) override;
  void OnImGui() const override;
  void LookAt(const glm::vec3& pos) override;
  [[nodiscard]] float GetPitch() const override;
  [[nodiscard]] float GetYaw() const override;

  void SetPosition(const glm::vec3& pos) override;
  void SetOrientation(float pitch, float yaw);

 private:
  float yaw_{0}, pitch_{0};
  void CalculateFront();
};
