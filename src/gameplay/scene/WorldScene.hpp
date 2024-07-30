#pragma once

#include "application/Scene.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkManager.hpp"
#include "renderer/ChunkRenderParams.hpp"

class WorldScene : public Scene {
 public:
  WorldScene(SceneManager& scene_manager, const std::string& world_name);

  void Update(double dt) override;
  void OnImGui() override;
  void Render(const Window& window) override;
  bool OnEvent(const SDL_Event& event) override;
  ~WorldScene() override;

  BlockDB block_db_;

  ChunkRenderParams chunk_render_params_;
  ChunkManager chunk_manager_;

 private:
  Player player_;
};
