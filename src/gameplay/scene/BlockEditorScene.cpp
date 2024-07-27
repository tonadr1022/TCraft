#include "BlockEditorScene.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "application/SceneManager.hpp"
#include "application/Window.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Renderer.hpp"
#include "resource/Image.hpp"
#include "resource/TextureManager.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

BlockEditorScene::BlockEditorScene(SceneManager& scene_manager) : Scene(scene_manager) {
  ZoneScoped;

  std::unordered_map<std::string, uint32_t> tex_name_to_idx;

  {
    ZoneScopedN("Load texture data");
    auto all_texture_names = BlockDB::GetAllBlockTexturesFromAllModels();
    std::vector<void*> all_texture_pixel_data;
    Image image;
    int tex_idx = 0;
    // TODO: handle diff sizes?
    for (const auto& tex_name : all_texture_names) {
      tex_name_to_idx[tex_name] = tex_idx++;
      util::LoadImage(image, GET_PATH("resources/textures/" + tex_name + ".png"), true);
      all_texture_pixel_data.emplace_back(image.pixels);
    }
    render_params_.chunk_tex_array_handle =
        TextureManager::Get().Create2dArray({.all_pixels_data = all_texture_pixel_data,
                                             .dims = glm::ivec2{32, 32},
                                             .generate_mipmaps = true,
                                             .internal_format = GL_RGBA8,
                                             .format = GL_RGBA,
                                             .filter_mode_min = GL_NEAREST_MIPMAP_LINEAR,
                                             .filter_mode_max = GL_NEAREST});
  }
  block_db_.Init(tex_name_to_idx, true);
  ChunkMesher mesher{block_db_.GetBlockData(), block_db_.GetMeshData()};

  for (uint32_t block = 1; block < block_db_.GetBlockData().size(); block++) {
    std::vector<ChunkVertex> vertices;
    std::vector<uint32_t> indices;
    mesher.GenerateBlock(vertices, indices, block);
    blocks_.emplace_back(
        SingleBlock{.mesh = {scene_manager.GetRenderer().AllocateChunk(vertices, indices)},
                    .pos = {0, block, 0},
                    .block = block});
  }
}

BlockEditorScene::~BlockEditorScene() {
  block_db_.WriteBlockData();
  TextureManager::Get().Remove2dArray(render_params_.chunk_tex_array_handle);
};

void BlockEditorScene::Render(Renderer& renderer, const Window& window) {
  ZoneScoped;
  scene_manager_.GetRenderer().RenderBlockEditor(
      *this, {
                 .vp_matrix = player_.GetCamera().GetProjection(window.GetAspectRatio()) *
                              player_.GetCamera().GetView(),
             });
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
  return player_.OnEvent(event);
}

void BlockEditorScene::Update(double dt) { player_.Update(dt); }
