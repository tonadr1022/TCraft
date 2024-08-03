#include "GamePlayer.hpp"

#include "gameplay/world/ChunkManager.hpp"

namespace {

float IntBound(float s, float ds) {
  bool s_is_integer = glm::round(s) == s;
  if (ds < 0 && s_is_integer) return 0;
  return (ds > 0 ? (s == 0.0f ? 1.0f : glm::ceil(s)) - s : s - glm::floor(s)) / glm::abs(ds);
}

}  // namespace

GamePlayer::GamePlayer(const ChunkManager& chunk_manager) : chunk_manager_(chunk_manager) {}

void GamePlayer::RayCast() {
  ZoneScoped;
  glm::vec3 direction = fps_camera_.GetFront();
  EASSERT_MSG(direction != glm::vec3(0), "Invalid front vector");

  // unit direction to step x,y,z
  glm::ivec3 step = glm::sign(direction);
  glm::vec3 origin = position_;
  glm::ivec3 block_pos = glm::floor(origin);
  glm::vec3 t_max = {IntBound(static_cast<float>(origin.x), direction.x),
                     IntBound(static_cast<float>(origin.y), direction.y),
                     IntBound(static_cast<float>(origin.z), direction.z)};
  glm::vec3 t_delta = static_cast<glm::vec3>(step) / direction;
  float radius = raycast_radius_ / glm::length(direction);

  while (true) {
    // incremental, find the axis where the distance to voxel edge along that axis is the least
    if (t_max.x < t_max.y) {
      if (t_max.x < t_max.z) {
        // x < z < y
        if (t_max.x > radius) break;
        block_pos.x += step.x;
        t_max.x += t_delta.x;
      } else {
        // z < x < y
        if (t_max.z > radius) break;
        block_pos.z += step.z;
        t_max.z += t_delta.z;
      }
    } else {
      if (t_max.y < t_max.z) {
        // y < z and x
        if (t_max.y > radius) break;
        block_pos.y += step.y;
        t_max.y += t_delta.y;

      } else {
        // z < y and x
        if (t_max.z > radius) break;
        block_pos.z += step.z;
        t_max.z += t_delta.z;
      }
    }
    ray_cast_air_pos_ = ray_cast_non_air_pos_;
    ray_cast_non_air_pos_ = block_pos;
    if (chunk_manager_.GetBlock(block_pos) != 0) {
      return;
    }
  }
}

const glm::ivec3& GamePlayer::GetRayCastBlockPos() const { return ray_cast_non_air_pos_; }

void GamePlayer::Update(double dt) {
  RayCast();
  Player::Update(dt);
}
