#pragma once

#include <glm/vec3.hpp>

#include "gameplay/Player.hpp"

class ChunkManager;

class GamePlayer : public Player {
 public:
  explicit GamePlayer(const ChunkManager& chunk_manager);
  void RayCast();
  void Update(double dt) override;
  const glm::ivec3& GetRayCastBlockPos() const;

 private:
  glm::ivec3 ray_cast_non_air_pos_;
  glm::ivec3 ray_cast_air_pos_;
  // bool active_aim_{false};
  float raycast_radius_{32.f};

  const ChunkManager& chunk_manager_;
};
