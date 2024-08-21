#pragma once

#include "application/Scene.hpp"
#include "gameplay/GamePlayer.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "renderer/Mesh.hpp"

class TextureMaterial;
class Texture;
class Window;
// class InventoryUI;

class WorldScene : public Scene {
 public:
  WorldScene(SceneManager& scene_manager, std::string_view path);

  void Update(double dt) override;
  void OnImGui() override;
  void Render() override;
  bool OnEvent(const SDL_Event& event) override;
  ~WorldScene() override;

  BlockDB block_db_;

 private:
  std::unique_ptr<ChunkManager> chunk_manager_;
  GamePlayer player_;
  bool loaded_{false};
  bool render_chunks_{false};
  uint32_t seed_{0};
  std::string directory_path_;

  std::shared_ptr<TextureMaterial> cross_hair_mat_;
  std::shared_ptr<TextureMaterial> chunk_state_tex_;
  std::shared_ptr<Texture> sun_skybox_;
  std::shared_ptr<TextureMaterial> test_mat_;

  std::vector<uint8_t> chunk_state_pixels_;
  glm::ivec2 prev_chunk_state_pixels_dims_{};
  bool first_frame_{true};
  bool show_chunk_map_{true};
  double time_{0};
  int chunk_map_display_y_level_{0};
  ChunkMapMode chunk_map_mode_{ChunkMapMode::ChunkState};
  // std::unique_ptr<InventoryUI> inv_ui_{nullptr};

  Mesh cube_mesh_;
  uint32_t chunk_tex_array_handle_;
};
