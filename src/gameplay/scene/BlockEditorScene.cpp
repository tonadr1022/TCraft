#include "BlockEditorScene.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "application/SceneManager.hpp"
#include "application/Window.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Renderer.hpp"
#include "resource/TextureManager.hpp"
#include "util/Paths.hpp"

BlockEditorScene::BlockEditorScene(SceneManager& scene_manager) : Scene(scene_manager) {
  ZoneScoped;
  name_ = "Block Editor";

  std::unordered_map<std::string, uint32_t> name_to_idx;
  render_params_.chunk_tex_array_handle = TextureManager::Get().Create2dArray(
      GET_PATH("resources/data/block/texture_2d_array.json"), name_to_idx);
  block_db_.Init(name_to_idx);
  block_db_.LoadAllBlockModels(name_to_idx);
  // add_model_tex_indexes_.fill(block_db_.block_defaults_.model_tex_index);
  ChunkMesher mesher{block_db_};
  for (int block = 0; block < 10; block++) {
    std::vector<ChunkVertex> vertices;
    std::vector<uint32_t> indices;
    mesher.GenerateBlock(vertices, indices, block);
    // scene_manager.GetRenderer().AllocateChunk(vertices, indices);
  }
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
  ZoneScoped;
  ImGui::Begin("Block Editor");

  if (ImGui::CollapsingHeader("Models", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::Button("Add Model")) {
      add_model_editor_open_ = true;
    }
    if (add_model_editor_open_) {
      static std::string model_name;
      ImGui::InputText("Name", &model_name);

      constexpr const static std::array<std::string, 3> TexTypes = {"all", "top_bottom", "unique"};
      constexpr const static std::array<std::string, 3> TexTypeNames = {"All", "Top Bottom",
                                                                        "Unique"};

      static int tex_type_idx = 0;
      ImGui::Text("Texture Type: ");
      ImGui::SameLine();
      for (int i = 0; i < 3; i++) {
        ImGui::BeginDisabled(tex_type_idx == i);
        if (ImGui::Button(TexTypeNames[i].data())) {
          tex_type_idx = i;
        }
        ImGui::EndDisabled();
        if (i < 2) ImGui::SameLine();
      }

      // type all
      if (tex_type_idx == 0) {
        static std::string tex_path_all;
        // type top bottom
      } else if (tex_type_idx == 1) {
        // type unique
      } else {
      }

      if (ImGui::Button("Cancel")) {
        model_name = "";
        add_model_editor_open_ = false;
        tex_type_idx = 0;
      }
      if (ImGui::Button("Save")) {
      }
    }
  }

  if (ImGui::CollapsingHeader("Blocks", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto& block_data_arr = block_db_.block_data_arr_;
    auto& block_model_names = block_db_.block_model_names_;

    static int edit_block_idx = -1;
    for (uint32_t i = 1; i < block_data_arr.size(); i++) {
      const auto& block_data = block_data_arr[i];
      if (ImGui::Button(block_data.name.data())) {
        edit_block_idx = i;
      }
    }

    static BlockData initial_edit_block_data;
    static std::string initial_edit_block_model;
    static bool first_edit = true;
    if (edit_block_idx != -1) {
      if (first_edit) {
        initial_edit_block_data = block_data_arr[edit_block_idx];
        initial_edit_block_model = block_model_names[edit_block_idx];
        first_edit = false;
      }

      // initialize the first time with block data name
      ImGui::InputText("Name", &block_data_arr[edit_block_idx].name);
      ImGui::Checkbox("Emits Light", &block_data_arr[edit_block_idx].emits_light);

      if (ImGui::BeginCombo("##Model", ("Model: " + block_model_names[edit_block_idx]).c_str())) {
        for (const auto& model : block_db_.all_block_model_names_) {
          if (model == block_model_names[edit_block_idx]) continue;
          if (ImGui::Selectable(model.data())) {
            block_model_names[edit_block_idx] = model;
          }
        }
        ImGui::EndCombo();
      }

      if (ImGui::Button("Cancel")) {
        block_data_arr[edit_block_idx] = initial_edit_block_data;
        block_model_names[edit_block_idx] = initial_edit_block_model;
        edit_block_idx = -1;
        first_edit = true;
      }
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

BlockEditorScene::~BlockEditorScene() { block_db_.WriteBlockData(); };
