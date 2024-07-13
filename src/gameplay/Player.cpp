#include "Player.hpp"

#include <SDL_keycode.h>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "application/Input.hpp"

void Player::Update(double dt) {
  float movement_offset = move_speed_ * dt;
  glm::vec3 movement(0.f);
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
  fps_camera_.position_ = position_;

  fps_camera_.Update(dt);
}
