#include "FPSCamera.hpp"

#include <SDL2/SDL_mouse.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>

#include "application/Settings.hpp"

glm::mat4 FPSCamera::GetProjection(float aspect_ratio) const {
  return glm::perspective(glm::radians(Settings::Get().fps_cam_fov_deg), aspect_ratio, near_plane_,
                          far_plane_);
}

glm::mat4 FPSCamera::GetView() const {
  return glm::lookAt(position_, position_ + front_, UpVector);
}

void FPSCamera::Update(double dt) {
  glm::ivec2 pos;
  static glm::ivec2 prev_pos{0};
  SDL_GetMouseState(&pos.x, &pos.y);
  glm::vec2 cursor_offset = pos - prev_pos;
  prev_pos = pos;

  float mouse_offset = 0.0001;
  yaw_ += cursor_offset.x * mouse_offset;
  pitch_ = glm::clamp(pitch_ - cursor_offset.y * mouse_offset, -89.0f, 89.0f);
  glm::vec3 front;
  front.x = glm::cos(glm::radians(yaw_)) * glm::cos(glm::radians(pitch_));
  front.y = glm::sin(glm::radians(pitch_));
  front.z = glm::sin(glm::radians(yaw_)) * glm::cos(glm::radians(pitch_));
  front_ = glm::normalize(front);
}
