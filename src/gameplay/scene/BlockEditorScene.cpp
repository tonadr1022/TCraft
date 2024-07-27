#include "BlockEditorScene.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <glm/ext/matrix_transform.hpp>

#include "application/SceneManager.hpp"
#include "application/Window.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "pch.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Renderer.hpp"
#include "resource/Image.hpp"
#include "resource/TextureManager.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

void BlockEditorScene::Reload() {
  {
    ZoneScopedN("Load texture data");
    // auto all_texture_names = BlockDB::GetAllBlockTexturesFromAllModels();
    std::vector<void*> all_texture_pixel_data;
    Image image;
    int tex_idx = 0;
    // TODO: handle diff sizes?
    for (const auto& file :
         std::filesystem::directory_iterator(GET_PATH("resources/textures/block"))) {
      // TODO: mcmeta animations or custom animation format
      if (file.path().extension() != ".png") {
        continue;
      }
      std::string tex_name =
          file.path().parent_path().filename().string() + "/" + file.path().stem().string();
      auto str = file.path().parent_path().string() + "/" + file.path().filename().string();
      util::LoadImage(image, str, true);
      if (image.width != 32 || image.height != 32) continue;
      tex_name_to_idx_[tex_name] = tex_idx++;
      all_texture_pixel_data.emplace_back(image.pixels);
    }

    // for (const auto& tex_name : all_texture_names) {
    //   tex_name_to_idx_[tex_name] = tex_idx++;
    //   util::LoadImage(image, GET_PATH("resources/textures/" + tex_name + ".png"), true);
    //   all_texture_pixel_data.emplace_back(image.pixels);
    // }
    render_params_.chunk_tex_array_handle =
        TextureManager::Get().Create2dArray({.all_pixels_data = all_texture_pixel_data,
                                             .dims = glm::ivec2{32, 32},
                                             .generate_mipmaps = true,
                                             .internal_format = GL_RGBA8,
                                             .format = GL_RGBA,
                                             .filter_mode_min = GL_NEAREST_MIPMAP_LINEAR,
                                             .filter_mode_max = GL_NEAREST});
  }
  block_db_.Init(tex_name_to_idx_, true, true);

  edit_model_type_all_ = {block_db_.block_defaults_.tex_name};
  edit_model_type_top_bot_ = {block_db_.block_defaults_.tex_name,
                              block_db_.block_defaults_.tex_name,
                              block_db_.block_defaults_.tex_name};
  edit_model_type_unique_ = {
      block_db_.block_defaults_.tex_name, block_db_.block_defaults_.tex_name,
      block_db_.block_defaults_.tex_name, block_db_.block_defaults_.tex_name,
      block_db_.block_defaults_.tex_name, block_db_.block_defaults_.tex_name,
  };

  HandleAddModelTextureChange();

  ChunkMesher mesher{block_db_.GetBlockData(), block_db_.GetMeshData()};
  for (uint32_t block = 1; block < block_db_.GetBlockData().size(); block++) {
    std::vector<ChunkVertex> vertices;
    std::vector<uint32_t> indices;
    mesher.GenerateBlock(vertices, indices, block);
    blocks_.emplace_back(SingleBlock{
        .mesh = {scene_manager_.GetRenderer().AllocateChunk(vertices, indices)},
        .pos = {2, block * 1.1 - static_cast<float>(block_db_.GetBlockData().size()) / 2.0f, -0.5},
        .block = block});
  }
}

void BlockEditorScene::HandleAddModelTextureChange() {
  scene_manager_.GetRenderer().Reset();
  if (block_mesh_data_.size() != 3) block_mesh_data_.resize(3);
  spdlog::info("reset {}", block_mesh_data_.size());
  uint32_t all_tex_idx = tex_name_to_idx_[edit_model_type_all_.tex_all];
  block_mesh_data_[0] = BlockMeshData{.texture_indices = {all_tex_idx, all_tex_idx, all_tex_idx,
                                                          all_tex_idx, all_tex_idx, all_tex_idx}};
  uint32_t top_tex_idx = tex_name_to_idx_[edit_model_type_top_bot_.tex_top];
  uint32_t bot_tex_idx = tex_name_to_idx_[edit_model_type_top_bot_.tex_bottom];
  uint32_t side_tex_idx = tex_name_to_idx_[edit_model_type_top_bot_.tex_side];
  block_mesh_data_[1] = BlockMeshData{.texture_indices = {side_tex_idx, side_tex_idx, top_tex_idx,
                                                          bot_tex_idx, side_tex_idx, side_tex_idx}};
  block_mesh_data_[2] = BlockMeshData{.texture_indices = {
                                          tex_name_to_idx_[edit_model_type_unique_.tex_pos_x],
                                          tex_name_to_idx_[edit_model_type_unique_.tex_neg_x],
                                          tex_name_to_idx_[edit_model_type_unique_.tex_pos_y],
                                          tex_name_to_idx_[edit_model_type_unique_.tex_neg_y],
                                          tex_name_to_idx_[edit_model_type_unique_.tex_pos_z],
                                          tex_name_to_idx_[edit_model_type_unique_.tex_neg_z],
                                      }};

  // TODO: refactor so block data either isn't necessary or used
  ChunkMesher block_mesher{block_db_.GetBlockData(), block_mesh_data_};
  for (uint32_t i = 0; i < 3; i++) {
    std::vector<ChunkVertex> vertices;
    std::vector<uint32_t> indices;
    block_mesher.GenerateBlock(vertices, indices, i);
    add_model_blocks_[i] = {
        .mesh = {scene_manager_.GetRenderer().AllocateChunk(vertices, indices)},
        .pos = {0, i * 1.1, 0},
        .block = static_cast<BlockType>(i),
    };
  }
}

BlockEditorScene::BlockEditorScene(SceneManager& scene_manager) : Scene(scene_manager) {
  ZoneScoped;
  Reload();
  player_.SetCameraFocused(true);
  player_.position_ = {-5, 2.5, 0};
}

BlockEditorScene::~BlockEditorScene() {
  block_db_.WriteBlockData();
  TextureManager::Get().Remove2dArray(render_params_.chunk_tex_array_handle);
};

void BlockEditorScene::Render(Renderer& renderer, const Window& window) {
  ZoneScoped;
  for (int i = 0; i < 3; i++) {
    renderer.SubmitChunkDrawCommand(glm::translate(glm::mat4{1}, glm::vec3{0, i * 1.2, 0}),
                                    add_model_blocks_[i].mesh.handle_);
  }

  // TODO: refactor into either render params or have scenes call submit functions themselves
  // for (const auto& block : blocks_) {
  //   // TODO: store model matrix and only update on change position
  //   renderer.SubmitChunkDrawCommand(glm::translate(glm::mat4{1}, glm::vec3(block.pos)),
  //                                   block.mesh.handle_);
  // }

  scene_manager_.GetRenderer().RenderBlockEditor(
      *this, {
                 .vp_matrix = player_.GetCamera().GetProjection(window.GetAspectRatio()) *
                              player_.GetCamera().GetView(),
             });
}

void BlockEditorScene::OnImGui() {
  ZoneScoped;

  ImGui::Begin("Block Editor");
  player_.OnImGui();

  if (ImGui::CollapsingHeader("Add Model", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("Add model");
    static std::string model_name;
    ImGui::InputText("Name", &model_name);

    constexpr const static std::array<std::string, 3> TexTypes = {"all", "top_bottom", "unique"};
    constexpr const static std::array<std::string, 3> TexTypeNames = {"All", "Top Bottom",
                                                                      "Unique"};
    for (int i = 0; i < 3; i++) {
      ImGui::BeginDisabled(add_model_tex_type_idx_ == i);
      if (ImGui::Button(TexTypeNames[i].data())) {
        add_model_tex_type_idx_ = i;
      }
      ImGui::EndDisabled();
      if (i < 2) ImGui::SameLine();
    }

    auto tex_select = [this](const char* title, std::string& str) {
      if (ImGui::BeginCombo(title, str.c_str())) {
        for (const auto& tex_name : block_db_.all_block_texture_names_) {
          if (ImGui::Selectable(tex_name.data())) {
            str = tex_name;
            HandleAddModelTextureChange();
          }
        }
        ImGui::EndCombo();
      }
    };

    if (add_model_tex_type_idx_ == 0) {
      tex_select("All", edit_model_type_all_.tex_all);
    } else if (add_model_tex_type_idx_ == 1) {
      tex_select("Top", edit_model_type_top_bot_.tex_top);
      tex_select("Bottom", edit_model_type_top_bot_.tex_bottom);
      tex_select("Side", edit_model_type_top_bot_.tex_side);
    } else {
      tex_select("Pos X", edit_model_type_unique_.tex_pos_x);
      tex_select("Neg X", edit_model_type_unique_.tex_neg_x);
      tex_select("Pos Y", edit_model_type_unique_.tex_pos_y);
      tex_select("Neg Y", edit_model_type_unique_.tex_neg_y);
      tex_select("Pos Z", edit_model_type_unique_.tex_pos_z);
      tex_select("Neg Z", edit_model_type_unique_.tex_neg_z);
    }

    if (ImGui::Button("Reset")) {
      edit_model_type_all_ = {};
      edit_model_type_top_bot_ = {};
      edit_model_type_unique_ = {};
      model_name = "";
      add_model_editor_open_ = false;
      add_model_tex_type_idx_ = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
    }
  }

  if (ImGui::CollapsingHeader("Blocks", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto& block_data_arr = block_db_.block_data_arr_;
    auto& block_model_names = block_db_.block_model_names_;

    static BlockData original_edit_block_data;
    static std::string original_edit_block_model_name;
    static int edit_block_idx = -1;
    for (uint32_t i = 1; i < block_data_arr.size(); i++) {
      const auto& block_data = block_data_arr[i];
      if (ImGui::Button(block_data.name.data())) {
        edit_block_idx = i;
        original_edit_block_data = block_data_arr[edit_block_idx];
        original_edit_block_model_name = block_model_names[edit_block_idx];
      }
    }

    static BlockModelType edit_block_model_type;
    if (edit_block_idx != -1) {
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
        block_data_arr[edit_block_idx] = original_edit_block_data;
        block_model_names[edit_block_idx] = original_edit_block_model_name;
        edit_block_idx = -1;
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
