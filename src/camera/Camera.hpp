#pragma once

#include <SDL_events.h>

#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

class Camera {
 public:
  [[nodiscard]] virtual glm::mat4 GetProjection(float aspect_ratio) const = 0;
  [[nodiscard]] virtual glm::mat4 GetView() const = 0;
  [[nodiscard]] inline const glm::vec3& GetFront() const { return front_; }

  virtual void Update(double dt) = 0;
  inline virtual bool OnEvent(const SDL_Event&) { return false; };
  virtual void SetPosition(const glm::vec3& pos) = 0;
  virtual void OnImGui() const = 0;
  virtual void LookAt(const glm::vec3& pos) = 0;

  static constexpr const glm::vec3 UpVector{0.f, 1.f, 0.f};

  bool first_mouse_{true};

 protected:
  glm::vec3 pos_{0, 0, 0};
  glm::vec3 front_{0, 0, -1};
  float near_plane_{0.1f}, far_plane_{1000.f};
};