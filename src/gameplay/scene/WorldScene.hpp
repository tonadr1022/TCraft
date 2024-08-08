#pragma once

#include "application/Scene.hpp"
#include "gameplay/GamePlayer.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkManager.hpp"
#include "renderer/ChunkRenderParams.hpp"

class TextureMaterial;
class Window;

class WorldScene : public Scene {
 public:
  WorldScene(SceneManager& scene_manager, std::string_view path);

  void Update(double dt) override;
  void OnImGui() override;
  void Render() override;
  bool OnEvent(const SDL_Event& event) override;
  ~WorldScene() override;

  BlockDB block_db_;

  ChunkRenderParams chunk_render_params_;
  std::unique_ptr<ChunkManager> chunk_manager_;

  std::shared_ptr<TextureMaterial> cross_hair_mat_;

 private:
  GamePlayer player_;
  uint32_t seed_{0};
  std::string directory_path_;
};
