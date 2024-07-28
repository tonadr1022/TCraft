#include "Player.hpp"

#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <imgui.h>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "application/Input.hpp"
#include "application/Window.hpp"

void Player::Update(double dt) {
  fps_camera_.position_ = position_;
  if (!camera_focused_) return;
  float movement_offset = move_speed_ * dt;
  glm::vec3 movement{0.f};
  if (Input::IsKeyDown(SDLK_w) || Input::IsKeyDown(SDLK_i)) {
    movement += fps_camera_.GetFront();
  }
  if (Input::IsKeyDown(SDLK_s) || Input::IsKeyDown(SDLK_k)) {
    movement -= fps_camera_.GetFront();
  }
  if (Input::IsKeyDown(SDLK_d) || Input::IsKeyDown(SDLK_l)) {
    movement += glm::normalize(glm::cross(fps_camera_.GetFront(), FPSCamera::UpVector));
  }
  if (Input::IsKeyDown(SDLK_a) || Input::IsKeyDown(SDLK_j)) {
    movement -= glm::normalize(glm::cross(fps_camera_.GetFront(), FPSCamera::UpVector));
  }
  if (Input::IsKeyDown(SDLK_y) || Input::IsKeyDown(SDLK_r)) {
    movement += FPSCamera::UpVector;
  }
  if (Input::IsKeyDown(SDLK_h) || Input::IsKeyDown(SDLK_f)) {
    movement -= FPSCamera::UpVector;
  }
  if (glm::length(movement) > 0) {
    movement = glm::normalize(movement) * movement_offset;
    position_ += movement;
  }
  fps_camera_.Update(dt);
}

void Player::OnImGui() const {
  ZoneScoped;
  ImGui::Begin("Player", nullptr,
               ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoFocusOnAppearing);
  ImGui::Text("Position %f, %f, %f", position_.x, position_.y, position_.z);
  ImGui::Text("Camera Focused: %s", camera_focused_ ? "true" : "false");
  if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
    fps_camera_.OnImGui();
  }
  ImGui::End();
}

bool Player::OnEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_f && event.key.keysym.mod & KMOD_ALT) {
        SetCameraFocused(!camera_focused_);
        return true;
      }
  }
  return false;
}

void Player::Init() const {
  SDL_SetRelativeMouseMode(camera_focused_ ? SDL_TRUE : SDL_FALSE);
  if (camera_focused_) {
    Window::DisableImGuiInputs();
  } else {
    Window::EnableImGuiInputs();
  }
}

void Player::SetCameraFocused(bool state) {
  camera_focused_ = state;
  SDL_SetRelativeMouseMode(camera_focused_ ? SDL_TRUE : SDL_FALSE);
  if (camera_focused_) {
    Window::DisableImGuiInputs();
  } else {
    Window::EnableImGuiInputs();
  }
  fps_camera_.first_mouse_ = true;
}
