#include "OrbitCamera.hpp"

#include <imgui.h>

#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "application/Input.hpp"
#include "application/SettingsManager.hpp"
#include "application/Window.hpp"

glm::mat4 OrbitCamera::GetProjection(float aspect_ratio) const {
  ZoneScoped;
  return glm::perspective(glm::radians(SettingsManager::Get().fps_cam_fov_deg), aspect_ratio,
                          near_plane_, far_plane_);
}

glm::mat4 OrbitCamera::GetView() const {
  ZoneScoped;
  return glm::lookAt(pos_, target_, UpVector);
}

bool OrbitCamera::OnEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_MOUSEWHEEL:
      distance_ += 0.1 * event.wheel.preciseY;
      UpdatePosition();
      return true;
  }
  return false;
}

void OrbitCamera::LookAt(const glm::vec3& pos) {
  target_ = pos;
  UpdatePosition();
}

void OrbitCamera::Update(double) {
  float sensitivity = SettingsManager::Get().orbit_mouse_sensitivity;
  auto mouse_pos = Window::Get().GetMousePosition();
  auto window_center = Window::Get().GetWindowCenter();
  glm::vec2 cursor_offset = mouse_pos - window_center;
  if (first_mouse_) {
    first_mouse_ = false;
    // Window::Get().CenterCursor();
    return;
  }
  // Window::Get().CenterCursor();
  if (cursor_offset.x != 0 || cursor_offset.y != 0) {
    if (Input::IsKeyDown(KMOD_RSHIFT)) {
      glm::vec3 move_offset = right_ * static_cast<float>(-cursor_offset.x) * sensitivity +
                              up_ * static_cast<float>(cursor_offset.y) * sensitivity;

      // TODO: speed here multiply
      target_ += move_offset;
    }

    azimuth_angle_ += static_cast<float>(cursor_offset.x) * sensitivity;
    polar_angle_ += static_cast<float>(cursor_offset.y) * sensitivity;
    polar_angle_ =
        glm::clamp(polar_angle_ + static_cast<float>(cursor_offset.y) * sensitivity, -89.0f, 89.0f);
    UpdatePosition();
  }
}

void OrbitCamera::UpdatePosition() {
  float azimuth_rad = glm::radians(azimuth_angle_);
  float polar_rad = glm::radians(polar_angle_);
  float sin_polar = glm::sin(polar_rad);
  float cos_polar = glm::cos(polar_rad);
  float cos_azimuth = glm::cos(azimuth_rad);
  float sin_azimuth = glm::sin(azimuth_rad);
  pos_.x = target_.x + distance_ * cos_polar * cos_azimuth;
  pos_.y = target_.y + distance_ * sin_polar;
  pos_.z = target_.z + distance_ * sin_azimuth * cos_polar;
  front_ = glm::normalize(target_ - pos_);
  right_ = glm::normalize(glm::cross(target_ - pos_, UpVector));
  up_ = glm::normalize(glm::cross(right_, target_ - pos_));
}

void OrbitCamera::SetPosition(const glm::vec3& pos) {
  auto dist_to_target = glm::distance(pos, target_);
  distance_ = dist_to_target;
  auto diff = pos - target_;
  polar_angle_ = glm::acos(diff.z / dist_to_target);
  float sign = diff.y > 0 ? 1 : diff.y < 0 ? -1 : 0;
  azimuth_angle_ = sign * glm::acos(diff.x / glm::sqrt(diff.x * diff.x + diff.y * diff.y));
  UpdatePosition();
}

void OrbitCamera::OnImGui() const {
  ImGui::Text("Position: %.1f, %.1f, %.1f", pos_.x, pos_.y, pos_.z);
  ImGui::Text("Front: %.2f, %.2f, %.2f", front_.x, front_.y, front_.z);
  ImGui::Text("Target: %.2f, %.2f, %.2f", target_.x, target_.y, target_.z);
}
