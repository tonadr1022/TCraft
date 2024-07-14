#include "FPSCamera.hpp"

#include <SDL2/SDL_mouse.h>
#include <imgui.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>

#include "application/Settings.hpp"
#include "application/Window.hpp"

glm::mat4 FPSCamera::GetProjection(float aspect_ratio) const {
  return glm::perspective(glm::radians(Settings::Get().fps_cam_fov_deg), aspect_ratio, near_plane_,
                          far_plane_);
}

glm::mat4 FPSCamera::GetView() const {
  return glm::lookAt(position_, position_ + front_, UpVector);
}

void FPSCamera::Update(double /* dt */) {
  auto mouse_pos = Window::Get().GetMousePosition();
  auto window_center = Window::Get().GetWindowCenter();
  glm::vec2 cursor_offset = mouse_pos - window_center;
  if (first_mouse_) {
    first_mouse_ = false;
    Window::Get().CenterCursor();
    return;
  }
  Window::Get().CenterCursor();

  float mouse_multiplier = 0.1 * Settings::Get().mouse_sensitivity;
  yaw_ += cursor_offset.x * mouse_multiplier;
  pitch_ = glm::clamp(pitch_ - cursor_offset.y * mouse_multiplier, -89.0f, 89.0f);
  CalculateFront();
}

void FPSCamera::CalculateFront() {
  glm::vec3 front;
  front.x = glm::cos(glm::radians(yaw_)) * glm::cos(glm::radians(pitch_));
  front.y = glm::sin(glm::radians(pitch_));
  front.z = glm::sin(glm::radians(yaw_)) * glm::cos(glm::radians(pitch_));
  front_ = glm::normalize(front);
}

void FPSCamera::OnImGui() const {
  ImGui::Text("Yaw: %.1f, Pitch: %.1f", yaw_, pitch_);
  ImGui::Text("Position: %.1f, %.1f, %.1f", position_.x, position_.y, position_.z);
  ImGui::Text("Front: %.2f, %.2f, %.2f", front_.x, front_.y, front_.z);
}

void FPSCamera::Save() {
  nlohmann::json j = {{"yaw", yaw_}, {"pitch", pitch_}};
  Settings::Get().SaveSetting(j, "fps_cam");
}

void FPSCamera::Load() {
  auto settings = Settings::Get().LoadSetting("fps_cam");
  yaw_ = settings["yaw"].get<float>();
  pitch_ = settings["pitch"].get<float>();
  CalculateFront();
}
