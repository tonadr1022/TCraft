#include "GamePlayer.hpp"

#include <imgui.h>

#include "Constants.hpp"
#include "application/Input.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkManager.hpp"

namespace {

float IntBound(float s, float ds) {
  if (abs(ds) <= 0.00001) return FLT_MAX;
  bool s_is_integer = glm::round(s) == s;
  if (ds < 0 && s_is_integer) return 0;
  return (ds > 0 ? (s == 0.0f ? 1.0f : glm::ceil(s)) - s : s - glm::floor(s)) / glm::abs(ds);
}

}  // namespace

GamePlayer::GamePlayer(ChunkManager& chunk_manager, const BlockDB& block_db)
    : chunk_manager_(chunk_manager), block_db_(block_db) {}

void GamePlayer::RayCast() {
  ZoneScoped;
  glm::vec3 direction = fps_camera_.GetFront();
  EASSERT_MSG(direction != glm::vec3(0), "Invalid front vector");

  // unit direction to step x,y,z
  glm::ivec3 step = glm::sign(direction);
  glm::vec3 origin = position_;
  glm::ivec3 block_pos = glm::floor(origin);
  glm::vec3 t_max = {IntBound(origin.x, direction.x), IntBound(origin.y, direction.y),
                     IntBound(origin.z, direction.z)};
  glm::vec3 t_delta = static_cast<glm::vec3>(step) / direction;
  if (std::abs(direction.x) < 0.0001)
    t_delta.x = 0;
  else
    t_delta.x = static_cast<float>(step.x) / direction.x;
  if (std::abs(direction.y) < 0.0001)
    t_delta.y = 0;
  else
    t_delta.y = static_cast<float>(step.y) / direction.y;
  if (std::abs(direction.z) < 0.0001)
    t_delta.z = 0;
  else
    t_delta.z = static_cast<float>(step.z) / direction.z;

  float radius = raycast_radius_ / glm::length(direction);

  while (true) {
    // incremental, find the axis where the distance to voxel edge along that axis is the least
    if (t_max.x < t_max.y) {
      if (t_max.x < t_max.z) {
        // x < z < y
        if (t_max.x > radius) {
          goto not_found;
        }
        block_pos.x += step.x;
        t_max.x += t_delta.x;
      } else {
        // z < x < y
        if (t_max.z > radius) {
          goto not_found;
        }
        block_pos.z += step.z;
        t_max.z += t_delta.z;
      }
    } else {
      if (t_max.y < t_max.z) {
        // y < z and x
        if (t_max.y > radius) {
          goto not_found;
        }
        block_pos.y += step.y;
        t_max.y += t_delta.y;

      } else {
        // z < y and x
        if (t_max.z > radius) {
          goto not_found;
        }
        block_pos.z += step.z;
        t_max.z += t_delta.z;
      }
    }
    ray_cast_air_pos_ = ray_cast_non_air_pos_;
    ray_cast_non_air_pos_ = block_pos;
    if (chunk_manager_.BlockPosExists(block_pos) && chunk_manager_.GetBlock(block_pos) != 0) {
      return;
    }
  }
not_found:
  ray_cast_air_pos_ = glm::NullIVec3;
  ray_cast_non_air_pos_ = glm::NullIVec3;
}

const glm::ivec3& GamePlayer::GetRayCastBlockPos() const { return ray_cast_non_air_pos_; }

void GamePlayer::Update(double dt) {
  RayCast();
  if (ray_cast_non_air_pos_ != glm::NullIVec3 &&
      (ray_cast_non_air_pos_ == prev_frame_ray_cast_non_air_pos_ ||
       prev_frame_ray_cast_non_air_pos_ == glm::NullIVec3)) {
    if (is_mining_) {
      elapsed_break_time_ += dt * mine_speed_;
      // if (elapsed_break_time_ >=
      // block_db_.GetBlockData()[chunk_manager_.GetBlock(ray_cast_non_air_pos_)].) {
      // TODO: user blockdb id for break time
      if (block_db_.GetBlockData()[0].id) {
      }
      if (elapsed_break_time_ >= 0.5) {
        chunk_manager_.SetBlock(ray_cast_non_air_pos_, 0);
      }
    }
  } else {
    elapsed_break_time_ = 0;
  }
  prev_frame_ray_cast_non_air_pos_ = ray_cast_non_air_pos_;
  Player::Update(dt);
}

void GamePlayer::OnImGui() {
  Player::OnImGui();
  ImGui::Begin("Player", nullptr,
               ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoFocusOnAppearing);
  ImGui::SliderFloat("Temp Mining Speed", &mine_speed_, 0.5, 10.0);
  if (chunk_manager_.BlockPosExists(ray_cast_air_pos_)) {
    ImGui::Text(
        "Block Type: %s",
        block_db_.GetBlockData()[chunk_manager_.GetBlock(ray_cast_non_air_pos_)].name.c_str());
  }
  ImGui::End();
}

bool GamePlayer::OnEvent(const SDL_Event& event) {
  if (Player::OnEvent(event)) return true;
  switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
      if (event.button.button == SDL_BUTTON_RIGHT) {
        if (ray_cast_air_pos_ != glm::NullIVec3) {
          // TODO: access held block in inventory
          chunk_manager_.SetBlock(ray_cast_air_pos_, 2);
        }
        return true;
      } else if (event.button.button == SDL_BUTTON_LEFT) {
        is_mining_ = true;
      }
      break;
    case SDL_MOUSEBUTTONUP:
      if (event.button.button == SDL_BUTTON_LEFT) {
        is_mining_ = false;
      }
  }
  return false;
}
