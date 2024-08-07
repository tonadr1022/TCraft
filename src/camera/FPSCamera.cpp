#include "FPSCamera.hpp"

#include <SDL2/SDL_mouse.h>
#include <imgui.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>

#include "application/SettingsManager.hpp"
#include "application/Window.hpp"

glm::mat4 FPSCamera::GetProjection(float aspect_ratio) const {
  ZoneScoped;
  return glm::perspective(glm::radians(SettingsManager::Get().fps_cam_fov_deg), aspect_ratio,
                          near_plane_, far_plane_);
}

glm::mat4 FPSCamera::GetView() const {
  ZoneScoped;
  return glm::lookAt(pos_, pos_ + front_, UpVector);
}

void FPSCamera::SetPosition(const glm::vec3& pos) { pos_ = pos; }

void FPSCamera::Update(double /* dt */) {
  ZoneScoped;
  auto mouse_pos = Window::Get().GetMousePosition();
  auto window_center = Window::Get().GetWindowCenter();
  glm::vec2 cursor_offset = mouse_pos - window_center;
  if (first_mouse_) {
    first_mouse_ = false;
    Window::Get().CenterCursor();
    return;
  }
  Window::Get().CenterCursor();

  float mouse_multiplier = 0.1 * SettingsManager::Get().mouse_sensitivity;
  yaw_ += cursor_offset.x * mouse_multiplier;
  pitch_ = glm::clamp(pitch_ - cursor_offset.y * mouse_multiplier, -89.0f, 89.0f);
  CalculateFront();
}

void FPSCamera::CalculateFront() {
  ZoneScoped;
  glm::vec3 front;
  front.x = glm::cos(glm::radians(yaw_)) * glm::cos(glm::radians(pitch_));
  front.y = glm::sin(glm::radians(pitch_));
  front.z = glm::sin(glm::radians(yaw_)) * glm::cos(glm::radians(pitch_));
  front_ = glm::normalize(front);
}

void FPSCamera::LookAt(const glm::vec3& pos) {
  ZoneScoped;
  front_ = glm::normalize(pos - pos_);
  yaw_ = glm::degrees(glm::atan(front_.z, front_.x));
  pitch_ = glm::degrees(glm::asin(front_.y));
  CalculateFront();
}

void FPSCamera::OnImGui() const {
  ZoneScoped;
  ImGui::Text("Yaw: %.1f, Pitch: %.1f", yaw_, pitch_);
  ImGui::Text("Position: %.1f, %.1f, %.1f", pos_.x, pos_.y, pos_.z);
  ImGui::Text("Front: %.2f, %.2f, %.2f", front_.x, front_.y, front_.z);
}

float FPSCamera::GetPitch() const { return pitch_; }
float FPSCamera::GetYaw() const { return yaw_; }

void FPSCamera::SetOrientation(float pitch, float yaw) {
  pitch_ = pitch;
  yaw_ = yaw;
  CalculateFront();
}
