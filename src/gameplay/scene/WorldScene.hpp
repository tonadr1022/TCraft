#pragma once

#include "gameplay/Player.hpp"
#include "gameplay/world/Chunk.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/vec3.hpp>
#include <unordered_map>

class BlockDB;
using ChunkMap = std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>>;

#include "application/Scene.hpp"

class WorldScene : public Scene {
 public:
  explicit WorldScene(SceneManager& scene_manager);
  void Update(double dt) override;
  void OnImGui() override;
  void Render(Renderer& renderer, const Window& window) override;
  bool OnEvent(const SDL_Event& event) override;
  ~WorldScene() override;

  std::unique_ptr<BlockDB> block_db_{nullptr};

  struct WorldRenderParams {
    uint32_t chunk_tex_array_handle{0};
  };
  WorldRenderParams world_render_params_;

 private:
  Player player_;
  friend class Renderer;
  ChunkMap chunk_map_;
  int load_distance_;
};
