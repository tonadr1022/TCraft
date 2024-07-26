#pragma once

#include "gameplay/Player.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkManager.hpp"

class Renderer;

#include "application/Scene.hpp"

class WorldScene : public Scene {
 public:
  WorldScene(SceneManager& scene_manager, const std::string& world_name);

  void Update(double dt) override;
  void OnImGui() override;
  void Render(Renderer& renderer, const Window& window) override;
  bool OnEvent(const SDL_Event& event) override;
  ~WorldScene() override;

  BlockDB block_db_;

  struct WorldRenderParams {
    uint32_t chunk_tex_array_handle{0};
  };
  WorldRenderParams world_render_params_;
  ChunkManager chunk_manager_;

 private:
  Player player_;
  friend class Renderer;
};
