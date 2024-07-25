#include "BlockEditorScene.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "application/SceneManager.hpp"
#include "application/Window.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "renderer/Renderer.hpp"
#include "resource/TextureManager.hpp"
#include "util/Paths.hpp"

BlockEditorScene::BlockEditorScene(SceneManager& scene_manager)
    : Scene(scene_manager), block_db_(std::make_unique<BlockDB>()) {
  ZoneScoped;
  name_ = "Block Editor";

  std::unordered_map<std::string, uint32_t> name_to_idx;
  render_params_.chunk_tex_array_handle = TextureManager::Get().Create2dArray(
      GET_PATH("resources/data/block/texture_2d_array.json"), name_to_idx);
  block_db_->Init(name_to_idx);
}

void BlockEditorScene::Render(Renderer& renderer, const Window& window) {
  ZoneScoped;
  RenderInfo render_info;
  render_info.window_dims = window.GetWindowSize();
  float aspect_ratio =
      static_cast<float>(render_info.window_dims.x) / static_cast<float>(render_info.window_dims.y);
  render_info.vp_matrix = fps_camera_.GetProjection(aspect_ratio) * fps_camera_.GetView();
  renderer.RenderBlockEditor(*this, render_info);
}

void BlockEditorScene::OnImGui() {
  ImGui::Begin("Block Editor");
  auto& block_data_arr = block_db_->block_data_arr_;
  auto& block_model_names = block_db_->block_model_names_;

  static int edit_block_idx = -1;
  for (uint32_t i = 1; i < block_data_arr.size(); i++) {
    const auto& block_data = block_data_arr[i];
    if (ImGui::Button(block_data.name.data())) {
      edit_block_idx = i;
    }
  }

  static BlockData initial_edit_block_data;
  static bool first_edit = true;
  if (edit_block_idx != -1) {
    if (first_edit) {
      initial_edit_block_data = block_data_arr[edit_block_idx];
      first_edit = false;
    }

    // initialize the first time with block data name
    ImGui::InputText("Name", &block_data_arr[edit_block_idx].name);
    ImGui::InputText("Model Name", &block_model_names[edit_block_idx]);
    ImGui::Checkbox("Emits Light", &block_data_arr[edit_block_idx].emits_light);
    ImGui::Text("Edit");
    if (ImGui::Button("Cancel")) {
      block_data_arr[edit_block_idx] = initial_edit_block_data;
      edit_block_idx = -1;
      first_edit = true;
    }
  }
  ImGui::End();
}

bool BlockEditorScene::OnEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_p && event.key.keysym.mod & KMOD_ALT) {
        scene_manager_.LoadScene("main_menu");
        return true;
      }
  }
  return false;
}

BlockEditorScene::~BlockEditorScene() { block_db_->WriteBlockData(); };
