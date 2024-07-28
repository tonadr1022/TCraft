#include "BlockEditorScene.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <glm/ext/matrix_transform.hpp>
#include <nlohmann/json.hpp>

#include "EAssert.hpp"
#include "application/SceneManager.hpp"
#include "application/Window.hpp"
#include "gameplay/world/Block.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "pch.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Renderer.hpp"
#include "resource/Image.hpp"
#include "resource/TextureManager.hpp"
#include "util/JsonUtil.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

void BlockEditorScene::Reload() {
  ZoneScoped;
  all_block_model_names_ = BlockDB::GetAllModelNames();
  all_block_model_names_set_ = {all_block_model_names_.begin(), all_block_model_names_.end()};
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

  block_mesh_data_.resize(3);
  ResetAddModelData();
  HandleAddModelTextureChange(BlockModelType::All);
  HandleAddModelTextureChange(BlockModelType::TopBottom);
  HandleAddModelTextureChange(BlockModelType::Unique);
}

void BlockEditorScene::HandleEditModelChange(BlockModelType type) {}

void BlockEditorScene::HandleAddModelTextureChange(BlockModelType type) {
  ZoneScoped;
  if (block_mesh_data_.size() != 3) block_mesh_data_.resize(3);
  if (type == BlockModelType::All) {
    uint32_t all_tex_idx = tex_name_to_idx_[add_model_data_all_.tex_all];
    block_mesh_data_[0] = BlockMeshData{.texture_indices = {all_tex_idx, all_tex_idx, all_tex_idx,
                                                            all_tex_idx, all_tex_idx, all_tex_idx}};

  } else if (type == BlockModelType::TopBottom) {
    uint32_t top_tex_idx = tex_name_to_idx_[add_model_data_top_bot_.tex_top];
    uint32_t bot_tex_idx = tex_name_to_idx_[add_model_data_top_bot_.tex_bottom];
    uint32_t side_tex_idx = tex_name_to_idx_[add_model_data_top_bot_.tex_side];
    block_mesh_data_[1] =
        BlockMeshData{.texture_indices = {side_tex_idx, side_tex_idx, top_tex_idx, bot_tex_idx,
                                          side_tex_idx, side_tex_idx}};
  } else {
    block_mesh_data_[2] = BlockMeshData{.texture_indices = {
                                            tex_name_to_idx_[add_model_data_unique_.tex_pos_x],
                                            tex_name_to_idx_[add_model_data_unique_.tex_neg_x],
                                            tex_name_to_idx_[add_model_data_unique_.tex_pos_y],
                                            tex_name_to_idx_[add_model_data_unique_.tex_neg_y],
                                            tex_name_to_idx_[add_model_data_unique_.tex_pos_z],
                                            tex_name_to_idx_[add_model_data_unique_.tex_neg_z],
                                        }};
  }

  // TODO: allow removal of meshes from the renderer buffers instead of resetting
  // scene_manager_.GetRenderer().Reset();
  // TODO: refactor so block data either isn't necessary or used
  auto i = static_cast<uint32_t>(type);
  ChunkMesher block_mesher{block_db_.GetBlockData(), block_mesh_data_};
  std::vector<ChunkVertex> vertices;
  std::vector<uint32_t> indices;
  block_mesher.GenerateBlock(vertices, indices, i);
  if (add_model_blocks_[i].mesh.IsAllocated()) {
    scene_manager_.GetRenderer().FreeChunk(add_model_blocks_[i].mesh.handle_);
  }
  add_model_blocks_[i] = {
      .mesh = {scene_manager_.GetRenderer().AllocateChunk(vertices, indices)},
      .pos = {0, i * 1.1, 0},
      .block = static_cast<BlockType>(i),
  };
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
  // TODO: store model matrix and only update on change position
  glm::mat4 model{1};
  model = glm::translate(model, {0.5, 0.5, 0.5});
  model = glm::rotate(model, block_rot_, {0, 1, 0});
  model = glm::translate(model, {-0.5, -0.5, -0.5});

  if (edit_mode_ == EditMode::AddModel) {
    auto& mesh = add_model_blocks_[static_cast<uint32_t>(add_model_type_)].mesh;
    EASSERT_MSG(mesh.IsAllocated(), "Add model mesh not allocated");
    renderer.SubmitChunkDrawCommand(model, mesh.handle_);
  } else if (edit_mode_ == EditMode::EditModel) {
    EASSERT_MSG(edit_model_block_.mesh.IsAllocated(), "Edit model not allocated");
    renderer.SubmitChunkDrawCommand(model, edit_model_block_.mesh.handle_);
  }

  scene_manager_.GetRenderer().RenderBlockEditor(
      *this, {
                 .vp_matrix = player_.GetCamera().GetProjection(window.GetAspectRatio()) *
                              player_.GetCamera().GetView(),
             });
}

void BlockEditorScene::ResetAddModelData() {
  add_model_data_all_ = {block_db_.block_defaults_.tex_name};
  add_model_data_top_bot_ = {block_db_.block_defaults_.tex_name, block_db_.block_defaults_.tex_name,
                             block_db_.block_defaults_.tex_name};
  add_model_data_unique_ = {
      block_db_.block_defaults_.tex_name, block_db_.block_defaults_.tex_name,
      block_db_.block_defaults_.tex_name, block_db_.block_defaults_.tex_name,
      block_db_.block_defaults_.tex_name, block_db_.block_defaults_.tex_name,
  };
}

void BlockEditorScene::OnImGui() {
  ZoneScoped;
  ImGui::Begin("Block Editor");
  player_.OnImGui();
  if (ImGui::BeginTabBar("MyTabBar")) {
    if (ImGui::BeginTabItem("Add Model")) {
      edit_mode_ = EditMode::AddModel;
      static std::string model_name;
      ImGui::InputText("Name", &model_name);

      constexpr const static std::array<std::string, 3> TexTypes = {"all", "top_bottom", "unique"};
      constexpr const static std::array<std::string, 3> TexTypeNames = {"All", "Top Bottom",
                                                                        "Unique"};
      for (uint32_t i = 0; i < 3; i++) {
        ImGui::BeginDisabled(static_cast<uint32_t>(add_model_type_) == i);
        if (ImGui::Button(TexTypeNames[i].data())) {
          add_model_type_ = static_cast<BlockModelType>(i);
        }
        ImGui::EndDisabled();
        if (i < 2) ImGui::SameLine();
      }

      auto tex_select = [this](const char* title, std::string& str) {
        if (ImGui::BeginCombo(title, str.c_str())) {
          for (const auto& tex_name : block_db_.all_block_texture_names_) {
            if (tex_name == str) continue;
            if (ImGui::Selectable(tex_name.data())) {
              str = tex_name;
              HandleAddModelTextureChange(add_model_type_);
            }
          }
          ImGui::EndCombo();
        }
      };

      if (add_model_type_ == BlockModelType::All) {
        tex_select("All##tex_select", add_model_data_all_.tex_all);
      } else if (add_model_type_ == BlockModelType::TopBottom) {
        tex_select("Top", add_model_data_top_bot_.tex_top);
        tex_select("Bottom", add_model_data_top_bot_.tex_bottom);
        tex_select("Side", add_model_data_top_bot_.tex_side);
      } else {
        tex_select("Pos X", add_model_data_unique_.tex_pos_x);
        tex_select("Neg X", add_model_data_unique_.tex_neg_x);
        tex_select("Pos Y", add_model_data_unique_.tex_pos_y);
        tex_select("Neg Y", add_model_data_unique_.tex_neg_y);
        tex_select("Pos Z", add_model_data_unique_.tex_pos_z);
        tex_select("Neg Z", add_model_data_unique_.tex_neg_z);
      }

      if (ImGui::Button("Reset")) {
        ResetAddModelData();
        model_name = "";
        add_model_editor_open_ = false;
        add_model_type_ = BlockModelType::All;
      }
      ImGui::SameLine();
      bool model_name_exists = all_block_model_names_set_.contains("block/" + model_name);
      ImGui::BeginDisabled(model_name_exists);
      if (ImGui::Button("Save")) {
        nlohmann::json j = nlohmann::json::object();
        j["type"] = TexTypes[static_cast<uint32_t>(add_model_type_)];
        if (add_model_type_ == BlockModelType::All) {
          j["textures"] = {{"all", add_model_data_all_.tex_all}};
        } else if (add_model_type_ == BlockModelType::TopBottom) {
          j["textures"] = {{"top", add_model_data_top_bot_.tex_top},
                           {"bottom", add_model_data_top_bot_.tex_bottom},
                           {"side", add_model_data_top_bot_.tex_side}};
        } else {
          j["textures"] = {
              {"posx", add_model_data_unique_.tex_pos_x},
              {"negx", add_model_data_unique_.tex_neg_x},
              {"posy", add_model_data_unique_.tex_pos_y},
              {"negy", add_model_data_unique_.tex_neg_y},
              {"posz", add_model_data_unique_.tex_pos_z},
              {"negz", add_model_data_unique_.tex_neg_z},
          };
        }
        json_util::WriteJson(j, GET_PATH("resources/data/model/block/" + model_name + ".json"));
      }

      ImGui::EndDisabled();
      if (model_name_exists) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(1.f, 0.f, 0.f)));
        ImGui::Text("Block model with this name already exists.");
        ImGui::PopStyleColor(1);
      }
      ImGui::EndTabItem();
    }
    if (all_block_model_names_.size() > 0) {
      if (ImGui::BeginTabItem("Edit Model")) {
        edit_mode_ = EditMode::EditModel;
        static std::string edit_model_name = all_block_model_names_[0];
        if (ImGui::BeginCombo("Model##edit_model_select", edit_model_name.c_str())) {
          for (const auto& model_name : all_block_model_names_) {
            if (model_name == edit_model_name) continue;
            if (ImGui::Selectable(model_name.data())) {
              auto model_data = BlockDB::LoadBlockModelDataFromName(model_name);
              if (!model_data.has_value()) {
                spdlog::error("Unable to load model data: {}", model_name);
                break;
              }
              edit_model_name = model_name;
              if (BlockModelDataAll* data = std::get_if<BlockModelDataAll>(&model_data.value())) {
                edit_model_data_all_.tex_all = data->tex_all;
                edit_model_type_ = BlockModelType::All;
              } else if (BlockModelDataTopBot* data =
                             std::get_if<BlockModelDataTopBot>(&model_data.value())) {
                edit_model_data_top_bot_.tex_top = data->tex_top;
                edit_model_data_top_bot_.tex_bottom = data->tex_bottom;
                edit_model_data_top_bot_.tex_side = data->tex_side;
                edit_model_type_ = BlockModelType::TopBottom;
              } else if (BlockModelDataUnique* data =
                             std::get_if<BlockModelDataUnique>(&model_data.value())) {
                edit_model_data_unique_.tex_pos_x = data->tex_pos_x;
                edit_model_data_unique_.tex_neg_x = data->tex_neg_x;
                edit_model_data_unique_.tex_pos_y = data->tex_pos_y;
                edit_model_data_unique_.tex_neg_y = data->tex_neg_y;
                edit_model_data_unique_.tex_pos_z = data->tex_pos_z;
                edit_model_data_unique_.tex_neg_z = data->tex_neg_z;
                edit_model_type_ = BlockModelType::Unique;
              }
            }
          }
          ImGui::EndCombo();
        }
        ImGui::EndTabItem();
      }
    }
    if (ImGui::BeginTabItem("Edit Block")) {
      edit_mode_ = EditMode::EditBlock;
      auto& block_data_arr = block_db_.block_data_arr_;
      auto& block_data_model_names = block_db_.block_model_names_;

      static BlockData original_edit_block_data;
      static std::string original_edit_block_model_name;
      static int edit_block_idx = -1;
      for (uint32_t i = 1; i < block_data_arr.size(); i++) {
        const auto& block_data = block_data_arr[i];
        if (ImGui::Button(block_data.name.data())) {
          edit_block_idx = i;
          original_edit_block_data = block_data_arr[edit_block_idx];
          original_edit_block_model_name = block_data_model_names[edit_block_idx];
        }
      }

      static BlockModelType edit_block_model_type;
      if (edit_block_idx != -1) {
        // initialize the first time with block data name
        ImGui::InputText("Name", &block_data_arr[edit_block_idx].name);
        ImGui::Checkbox("Emits Light", &block_data_arr[edit_block_idx].emits_light);

        if (ImGui::BeginCombo("##Model",
                              ("Model: " + block_data_model_names[edit_block_idx]).c_str())) {
          for (const auto& model : all_block_model_names_) {
            if (model == block_data_model_names[edit_block_idx]) continue;
            if (ImGui::Selectable(model.data())) {
              block_data_model_names[edit_block_idx] = model;
            }
          }
          ImGui::EndCombo();
        }

        if (ImGui::Button("Cancel")) {
          block_data_arr[edit_block_idx] = original_edit_block_data;
          block_data_model_names[edit_block_idx] = original_edit_block_model_name;
          edit_block_idx = -1;
        }
      }
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
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

void BlockEditorScene::Update(double dt) {
  player_.Update(dt);
  block_rot_ += dt * 0.001;
}
