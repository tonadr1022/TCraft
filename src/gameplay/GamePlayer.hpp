#pragma once

#include <glm/vec3.hpp>

#include "gameplay/Player.hpp"

class ChunkManager;
class BlockDB;

class GamePlayer : public Player {
 public:
  explicit GamePlayer(ChunkManager& chunk_manager, const BlockDB& block_db);
  void RayCast();
  bool OnEvent(const SDL_Event& event) override;
  void Update(double dt) override;
  void OnImGui() override;
  [[nodiscard]] const glm::ivec3& GetRayCastBlockPos() const;
  [[nodiscard]] const glm::ivec3& GetAirRayCastBlockPos() const;

 private:
  glm::ivec3 ray_cast_non_air_pos_;
  glm::ivec3 ray_cast_air_pos_;
  glm::ivec3 prev_frame_ray_cast_non_air_pos_;
  double elapsed_break_time_{0};
  bool is_mining_{false};
  float mine_speed_{1};
  // bool active_aim_{false};
  float raycast_radius_{32.f};

  ChunkManager& chunk_manager_;
  const BlockDB& block_db_;
};
