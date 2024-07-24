#include "BlockEditorScene.hpp"

#include "application/Window.hpp"
#include "renderer/Renderer.hpp"

BlockEditorScene::BlockEditorScene(SceneManager& scene_manager) : Scene(scene_manager) {}

void BlockEditorScene::Render(Renderer& renderer, const Window& window) {
  ZoneScoped;
  RenderInfo render_info;
  render_info.window_dims = window.GetWindowSize();
  float aspect_ratio =
      static_cast<float>(render_info.window_dims.x) / static_cast<float>(render_info.window_dims.y);
  render_info.vp_matrix = fps_camera_.GetProjection(aspect_ratio) * fps_camera_.GetView();
  renderer.RenderBlockEditor(*this, render_info);
}
void BlockEditorScene::OnImGui() {}

bool BlockEditorScene::OnEvent(const SDL_Event& /*event*/) { return false; }

BlockEditorScene::~BlockEditorScene() = default;
