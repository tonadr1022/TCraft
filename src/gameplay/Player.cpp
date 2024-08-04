#include "Player.hpp"

#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <imgui.h>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "application/Input.hpp"
#include "application/Window.hpp"

void Player::Init() const {
  SDL_SetRelativeMouseMode(camera_focused_ ? SDL_TRUE : SDL_FALSE);
  if (camera_focused_) {
    Window::DisableImGuiInputs();
  } else {
    Window::EnableImGuiInputs();
  }
}

const glm::vec3& Player::Position() const { return position_; }

void Player::SetPosition(const glm::vec3& pos) {
  position_ = pos;
  fps_camera_.SetPosition(pos);
  orbit_camera_.SetPosition(pos);
}

void Player::Update(double dt) {
  if (!camera_focused_) return;
  if (camera_mode == CameraMode::FPS) {
    fps_camera_.SetPosition(position_);
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
  } else {
    orbit_camera_.Update(dt);
  }
}

void Player::OnImGui() const {
  ZoneScoped;
  ImGui::Begin("Player", nullptr,
               ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoFocusOnAppearing);
  ImGui::Text("Position %f, %f, %f", position_.x, position_.y, position_.z);
  ImGui::Text("Camera Focused: %s", camera_focused_ ? "true" : "false");
  if (camera_mode == CameraMode::FPS) {
    if (ImGui::CollapsingHeader("FPS Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
      fps_camera_.OnImGui();
    }
  } else {
    if (ImGui::CollapsingHeader("Orbit Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
      orbit_camera_.OnImGui();
    }
  }
  ImGui::End();
}
bool Player::OnEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_KEYDOWN:
      switch (event.key.keysym.sym) {
        case SDLK_f:
          if (event.key.keysym.mod & KMOD_ALT) {
            SetCameraFocused(!camera_focused_);
            return true;
          }
        case SDLK_m:
          if (event.key.keysym.mod & KMOD_ALT) {
            camera_mode = camera_mode == CameraMode::FPS ? CameraMode::Orbit : CameraMode::FPS;
            return true;
          }
      }
  }
  if (camera_mode == CameraMode::FPS) {
    return fps_camera_.OnEvent(event);
  }
  return orbit_camera_.OnEvent(event);
}

bool Player::GetCameraFocused() const { return camera_focused_; }

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

Camera& Player::GetCamera() {
  if (camera_mode == CameraMode::FPS) return fps_camera_;
  return orbit_camera_;
}

void Player::LookAt(const glm::vec3& pos) {
  fps_camera_.LookAt(pos);
  orbit_camera_.LookAt(pos);
}
