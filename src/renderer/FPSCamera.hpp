#pragma once

#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

class FPSCamera {
 public:
  [[nodiscard]] glm::mat4 GetProjection(float aspect_ratio) const;
  [[nodiscard]] glm::mat4 GetView() const;

  static constexpr const glm::vec3 UpVector{0.f, 1.f, 0.f};
  void Update(double dt);
  void OnImGui() const;

  void Save();
  void Load();

  glm::vec3 position_;
  [[nodiscard]] const glm::vec3& GetFront() const { return front_; }

  bool first_mouse_{true};

 private:
  glm::vec3 front_;
  float yaw_, pitch_;
  float near_plane_{0.1f}, far_plane_{1000.f};
  void CalculateFront();
};
