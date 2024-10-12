#pragma once

#include "application/Scene.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/Terrain.hpp"
#include "renderer/opengl/Buffer.hpp"
#include "renderer/opengl/VertexArray.hpp"

class Texture;
class ShadowScene : public Scene {
 public:
  explicit ShadowScene(SceneManager& scene_manager);
  ~ShadowScene() override;
  void OnImGui() override;
  void Update(double dt) override;
  bool OnEvent(const SDL_Event& event) override;
  void Render() override;

 private:
  Player player_;
  VertexArray cube_vao, plane_vao;
  Buffer cube_vbo, plane_vbo;
  BlockDB block_db_;
  Terrain terrain_;
  std::shared_ptr<Texture> chunk_tex_array_;
  void render_cube();
  void render_plane();
};
