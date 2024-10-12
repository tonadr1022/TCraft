#pragma once

#include "application/Scene.hpp"
#include "gameplay/GamePlayer.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/TextureAtlas.hpp"

class TextureMaterial;
class Texture;
class Window;

class WorldScene : public Scene {
 public:
  WorldScene(SceneManager& scene_manager, const std::string& directory_path);

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
  ChunkMapMode chunk_map_mode_{ChunkMapMode::kChunkState};
  glm::vec3 sun_dir_ = {0.4, 1, 0.4};
  float sun_circle_animate_height_{1.f};
  bool animate_time_of_day_{false};
  enum class SunMovementMode { kRegular, kCircleDaytime };
  SunMovementMode sun_movement_mode_;

  SquareTextureAtlas icon_texture_atlas_;
  Mesh cube_mesh_;
  std::shared_ptr<Texture> chunk_tex_array_;
  void DrawInventory();
};
