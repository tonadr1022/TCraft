#pragma once

#include "gameplay/Player.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <string_view>
#include <unordered_map>

#include "Chunk.hpp"

using ChunkMap = std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>>;
class Player;

class World {
 public:
  void Update(double dt);
  void Load(std::string_view path);
  void Save();
  void OnImGui();
  void Render();
  Player& GetPlayer() { return player_; }
  bool OnEvent(const SDL_Event& event);

 private:
  Player player_;
  friend class Renderer;
  ChunkMap chunk_map_;
  int load_distance_;
};
