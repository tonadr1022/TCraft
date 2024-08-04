#pragma once

#include <glm/vec3.hpp>

#include "gameplay/Player.hpp"

class ChunkManager;
class BlockDB;

class GamePlayer : public Player {
 public:
  explicit GamePlayer(ChunkManager& chunk_manager, const BlockDB& block_db);
  void RayCast();
  void Update(double dt) override;
  [[nodiscard]] const glm::ivec3& GetRayCastBlockPos() const;

 private:
  glm::ivec3 ray_cast_non_air_pos_;
  glm::ivec3 ray_cast_air_pos_;
  glm::ivec3 prev_frame_ray_cast_non_air_pos_;
  double elapsed_break_time_{0};
  // bool active_aim_{false};
  float raycast_radius_{32.f};

  ChunkManager& chunk_manager_;
  const BlockDB& block_db_;
};
