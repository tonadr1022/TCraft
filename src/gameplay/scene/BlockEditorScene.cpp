#include "BlockEditorScene.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "application/SceneManager.hpp"
#include "application/Window.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "pch.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Renderer.hpp"
#include "resource/Image.hpp"
#include "resource/MaterialManager.hpp"
#include "resource/TextureManager.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

void BlockEditorScene::Reload() {
  ZoneScoped;
  {
    ZoneScopedN("UI");
    // TODO: make this more streamlined across scenes
    // cross_hair_tex_ = TextureManager::Get().Load({.filepath =
    // GET_TEXTURE_PATH("crosshair.png")});
    cross_hair_mat_ =
        MaterialManager::Get().LoadTextureMaterial({.filepath = GET_TEXTURE_PATH("crosshair.png")});
    // crosshair_mat_handle_ =
    //     Renderer::Get().AllocateTextureMaterial(cross_hair_tex_->BindlessHandle());

    // for (int j = 0; j < 10; j++) {
    //   for (int i = 0; i < 10; i++) {
    //     Renderer::Get().AddStaticQuad(crosshair_mat_handle_, {i * 10, j * 10}, {50, 50});
    //   }
    // }
  }

  all_block_model_names_ = BlockDB::GetAllBlockModelNames();
  std::sort(all_block_model_names_.begin(), all_block_model_names_.end());
  all_block_model_names_set_ = {all_block_model_names_.begin(), all_block_model_names_.end()};
  all_block_texture_names_ = BlockDB::GetAllTextureNames();
  std::sort(all_block_texture_names_.begin(), all_block_texture_names_.end());
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
      util::LoadImage(image, str);
      if (image.width != 32 || image.height != 32) continue;
      tex_name_to_idx_[tex_name] = tex_idx++;
      all_texture_pixel_data.emplace_back(image.pixels);
    }

    chunk_render_params_.chunk_tex_array_handle =
        TextureManager::Get().Create2dArray({.all_pixels_data = all_texture_pixel_data,
                                             .dims = glm::ivec2{32, 32},
                                             .generate_mipmaps = true,
                                             .internal_format = GL_RGBA8,
                                             .format = GL_RGBA,
                                             .filter_mode_min = GL_NEAREST_MIPMAP_LINEAR,
                                             .filter_mode_max = GL_NEAREST});
  }
  block_db_.Init();
  block_db_.LoadMeshData(tex_name_to_idx_);

  ResetAddModelData();
  HandleAddModelTextureChange(BlockModelType::All);
  HandleAddModelTextureChange(BlockModelType::TopBottom);
  HandleAddModelTextureChange(BlockModelType::Unique);
  HandleEditModelChange();

  std::vector<ChunkVertex> vertices;
  std::vector<uint32_t> indices;
  int num_blocks = block_db_.GetBlockData().size();
  for (int i = 1; i < num_blocks; i++) {
    vertices.clear();
    indices.clear();
    ChunkMesher::GenerateBlock(vertices, indices, block_db_.GetMeshData()[i].texture_indices);
    blocks_.emplace_back(SingleBlock{
        .pos = {(-num_blocks + i), 0, 0},
        .mesh = {vertices, indices},
        .mesh_data = {},
    });
  }
  original_edit_block_data_ = block_db_.block_data_arr_[1];
  original_edit_block_model_name_ = block_db_.block_model_names_[1];
}

void BlockEditorScene::HandleEditModelChange() {
  ZoneScoped;
  if (edit_model_type_ == BlockModelType::All) {
    uint32_t all_tex_idx = tex_name_to_idx_[edit_model_data_all_.tex_all];
    edit_model_block_.mesh_data = {.texture_indices = {all_tex_idx, all_tex_idx, all_tex_idx,
                                                       all_tex_idx, all_tex_idx, all_tex_idx}};
  } else if (edit_model_type_ == BlockModelType::TopBottom) {
    uint32_t top_tex_idx = tex_name_to_idx_[edit_model_data_top_bot_.tex_top];
    uint32_t bot_tex_idx = tex_name_to_idx_[edit_model_data_top_bot_.tex_bottom];
    uint32_t side_tex_idx = tex_name_to_idx_[edit_model_data_top_bot_.tex_side];
    edit_model_block_.mesh_data = {.texture_indices = {side_tex_idx, side_tex_idx, top_tex_idx,
                                                       bot_tex_idx, side_tex_idx, side_tex_idx}};
  } else {
    edit_model_block_.mesh_data = {.texture_indices = {
                                       tex_name_to_idx_[edit_model_data_unique_.tex_pos_x],
                                       tex_name_to_idx_[edit_model_data_unique_.tex_neg_x],
                                       tex_name_to_idx_[edit_model_data_unique_.tex_pos_y],
                                       tex_name_to_idx_[edit_model_data_unique_.tex_neg_y],
                                       tex_name_to_idx_[edit_model_data_unique_.tex_pos_z],
                                       tex_name_to_idx_[edit_model_data_unique_.tex_neg_z],
                                   }};
  }

  std::vector<ChunkVertex> vertices;
  std::vector<uint32_t> indices;
  ChunkMesher::GenerateBlock(vertices, indices, edit_model_block_.mesh_data.texture_indices);
  edit_model_block_ = {};
  edit_model_block_.mesh.Allocate(vertices, indices);
}

void BlockEditorScene::HandleAddModelTextureChange(BlockModelType type) {
  ZoneScoped;
  if (type == BlockModelType::All) {
    uint32_t all_tex_idx = tex_name_to_idx_[add_model_data_all_.tex_all];
    add_model_blocks_[0].mesh_data = {.texture_indices = {all_tex_idx, all_tex_idx, all_tex_idx,
                                                          all_tex_idx, all_tex_idx, all_tex_idx}};

  } else if (type == BlockModelType::TopBottom) {
    uint32_t top_tex_idx = tex_name_to_idx_[add_model_data_top_bot_.tex_top];
    uint32_t bot_tex_idx = tex_name_to_idx_[add_model_data_top_bot_.tex_bottom];
    uint32_t side_tex_idx = tex_name_to_idx_[add_model_data_top_bot_.tex_side];
    add_model_blocks_[1].mesh_data = {.texture_indices = {side_tex_idx, side_tex_idx, top_tex_idx,
                                                          bot_tex_idx, side_tex_idx, side_tex_idx}};
  } else {
    add_model_blocks_[2].mesh_data = {.texture_indices = {
                                          tex_name_to_idx_[add_model_data_unique_.tex_pos_x],
                                          tex_name_to_idx_[add_model_data_unique_.tex_neg_x],
                                          tex_name_to_idx_[add_model_data_unique_.tex_pos_y],
                                          tex_name_to_idx_[add_model_data_unique_.tex_neg_y],
                                          tex_name_to_idx_[add_model_data_unique_.tex_pos_z],
                                          tex_name_to_idx_[add_model_data_unique_.tex_neg_z],
                                      }};
  }

  auto i = static_cast<uint32_t>(type);
  std::vector<ChunkVertex> vertices;
  std::vector<uint32_t> indices;
  ChunkMesher::GenerateBlock(vertices, indices, add_model_blocks_[i].mesh_data.texture_indices);
  add_model_blocks_[i] = {};
  add_model_blocks_[i].mesh.Allocate(vertices, indices);
}

BlockEditorScene::BlockEditorScene(SceneManager& scene_manager) : Scene(scene_manager) {
  ZoneScoped;
  Reload();
  player_.SetCameraFocused(false);
  player_.SetPosition({0, 0.5, 2});
  player_.LookAt({0.5f, 0.5f, 0.5f});
  player_.camera_mode = Player::CameraMode::Orbit;
}

BlockEditorScene::~BlockEditorScene() {
  TextureManager::Get().Remove2dArray(chunk_render_params_.chunk_tex_array_handle);
};

void BlockEditorScene::Render() {
  ZoneScoped;
  // TODO: store model matrix and only update on change position?
  glm::mat4 model{1};

  {
    ZoneScopedN("Block render");
    model = glm::translate(model, {0.5, 0.5, 0.5});
    model = glm::rotate(model, block_rot_, {0, 1, 0});
    model = glm::translate(model, {-0.5, -0.5, -0.5});
    auto win_center = window_.GetWindowCenter();
    Renderer::Get().DrawQuad(cross_hair_mat_->Handle(), {win_center.x, win_center.y}, {20, 20});
    if (edit_mode_ == EditMode::AddModel) {
      auto& mesh = add_model_blocks_[static_cast<uint32_t>(add_model_type_)].mesh;
      EASSERT_MSG(mesh.IsAllocated(), "Add model mesh not allocated");
      Renderer::Get().SubmitChunkDrawCommand(model, mesh.Handle());
    } else if (edit_mode_ == EditMode::EditModel) {
      EASSERT_MSG(edit_model_block_.mesh.IsAllocated(), "Edit model not allocated");
      Renderer::Get().SubmitChunkDrawCommand(model, edit_model_block_.mesh.Handle());
    } else if (edit_mode_ == EditMode::EditBlock) {
      for (const auto& block : blocks_) {
        EASSERT_MSG(block.mesh.IsAllocated(), "model not allocated");
        Renderer::Get().SubmitChunkDrawCommand(glm::translate(glm::mat4{1}, block.pos),
                                               block.mesh.Handle());
      }
    }
  }
  Renderer::Get().RenderWorld(
      chunk_render_params_,
      {
          .vp_matrix = player_.GetCamera().GetProjection(window_.GetAspectRatio()) *
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

void BlockEditorScene::TexSelectMenu(EditMode mode) {
  auto tex_select = [this, &mode](const char* title, std::string& str) {
    if (ImGui::BeginCombo(title, str.c_str())) {
      for (const auto& tex_name : all_block_texture_names_) {
        if (tex_name == str) continue;
        if (ImGui::Selectable(tex_name.data())) {
          str = tex_name;
          if (mode == EditMode::AddModel)
            HandleAddModelTextureChange(add_model_type_);
          else if (mode == EditMode::EditModel)
            HandleEditModelChange();
        }
      }
      ImGui::EndCombo();
    };
  };

  BlockModelDataAll& model_data_all =
      edit_mode_ == EditMode::EditModel ? edit_model_data_all_ : add_model_data_all_;
  BlockModelDataTopBot& model_data_top_bot =
      edit_mode_ == EditMode::EditModel ? edit_model_data_top_bot_ : add_model_data_top_bot_;
  BlockModelDataUnique& model_data_unique =
      edit_mode_ == EditMode::EditModel ? edit_model_data_unique_ : add_model_data_unique_;
  BlockModelType& model_type =
      edit_mode_ == EditMode::EditModel ? edit_model_type_ : add_model_type_;

  if (model_type == BlockModelType::All) {
    tex_select("All##tex_select", model_data_all.tex_all);
  } else if (model_type == BlockModelType::TopBottom) {
    tex_select("Top", model_data_top_bot.tex_top);
    tex_select("Bottom", model_data_top_bot.tex_bottom);
    tex_select("Side", model_data_top_bot.tex_side);
  } else {
    tex_select("Pos X", model_data_unique.tex_pos_x);
    tex_select("Neg X", model_data_unique.tex_neg_x);
    tex_select("Pos Y", model_data_unique.tex_pos_y);
    tex_select("Neg Y", model_data_unique.tex_neg_y);
    tex_select("Pos Z", model_data_unique.tex_pos_z);
    tex_select("Neg Z", model_data_unique.tex_neg_z);
  }
}

void BlockEditorScene::OnImGui() {
  ZoneScoped;
  ImGui::Begin("Block Editor");
  player_.OnImGui();
  if (ImGui::BeginTabBar("MyTabBar")) {
    ZoneScopedN("Tab Bar");
    if (ImGui::BeginTabItem("Add Model")) {
      ZoneScopedN("Add Model tab");
      edit_mode_ = EditMode::AddModel;
      static std::string model_name;
      ImGui::InputText("Name", &model_name);

      ImGui::Text("Textures");

      for (uint32_t i = 0; i < 3; i++) {
        ImGui::BeginDisabled(static_cast<uint32_t>(add_model_type_) == i);
        if (ImGui::Button(TexTypeNames[i].data())) {
          add_model_type_ = static_cast<BlockModelType>(i);
        }
        ImGui::EndDisabled();
        if (i < 2) ImGui::SameLine();
      }

      TexSelectMenu(edit_mode_);

      if (ImGui::Button("Reset")) {
        ResetAddModelData();
        model_name = "";
        add_model_editor_open_ = false;
        add_model_type_ = BlockModelType::All;
      }
      ImGui::SameLine();
      bool model_name_exists = all_block_model_names_set_.contains(model_name);
      ImGui::BeginDisabled(model_name_exists);
      if (ImGui::Button("Save")) {
        std::string path = GET_PATH("resources/data/model/block/" + model_name + ".json");
        if (add_model_type_ == BlockModelType::All) {
          BlockDB::WriteBlockModelTypeAll(add_model_data_all_, path);
        } else if (add_model_type_ == BlockModelType::TopBottom) {
          BlockDB::WriteBlockModelTypeTopBot(add_model_data_top_bot_, path);
        } else {
          BlockDB::WriteBlockModelTypeUnique(add_model_data_unique_, path);
        }
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
        ZoneScopedN("Edit Model tab");
        edit_mode_ = EditMode::EditModel;
        static std::string edit_model_name = all_block_model_names_[0];
        static auto model_data = BlockDB::LoadBlockModelDataFromName("block/" + edit_model_name);
        static BlockModelDataAll original_edit_model_data_all;
        static BlockModelDataTopBot original_edit_model_top_bot;
        static BlockModelDataUnique original_edit_model_unique;
        static bool first_edit = true;

        auto set_data = [this](std::optional<BlockModelData>& model_data) {
          if (BlockModelDataAll* data = std::get_if<BlockModelDataAll>(&model_data.value())) {
            edit_model_type_ = BlockModelType::All;
            edit_model_data_all_.tex_all = data->tex_all;
          } else {
            edit_model_data_all_ = {block_db_.block_defaults_.tex_name};
          }
          if (BlockModelDataTopBot* data = std::get_if<BlockModelDataTopBot>(&model_data.value())) {
            edit_model_type_ = BlockModelType::TopBottom;
            edit_model_data_top_bot_.tex_top = data->tex_top;
            edit_model_data_top_bot_.tex_bottom = data->tex_bottom;
            edit_model_data_top_bot_.tex_side = data->tex_side;
          } else {
            edit_model_type_ = BlockModelType::TopBottom;
            edit_model_data_top_bot_ = {block_db_.block_defaults_.tex_name,
                                        block_db_.block_defaults_.tex_name,
                                        block_db_.block_defaults_.tex_name};
          }
          if (BlockModelDataUnique* data = std::get_if<BlockModelDataUnique>(&model_data.value())) {
            edit_model_type_ = BlockModelType::Unique;
            edit_model_data_unique_.tex_pos_x = data->tex_pos_x;
            edit_model_data_unique_.tex_neg_x = data->tex_neg_x;
            edit_model_data_unique_.tex_pos_y = data->tex_pos_y;
            edit_model_data_unique_.tex_neg_y = data->tex_neg_y;
            edit_model_data_unique_.tex_pos_z = data->tex_pos_z;
            edit_model_data_unique_.tex_neg_z = data->tex_neg_z;
          } else {
            edit_model_data_unique_ = {
                block_db_.block_defaults_.tex_name, block_db_.block_defaults_.tex_name,
                block_db_.block_defaults_.tex_name, block_db_.block_defaults_.tex_name,
                block_db_.block_defaults_.tex_name, block_db_.block_defaults_.tex_name,
            };
          }
          original_edit_model_data_all = edit_model_data_all_;
          original_edit_model_unique = edit_model_data_unique_;
          original_edit_model_top_bot = edit_model_data_top_bot_;
        };

        if (first_edit) {
          set_data(model_data);
          HandleEditModelChange();
          first_edit = false;
        }

        if (ImGui::BeginCombo("Model##edit_model_select", edit_model_name.c_str())) {
          for (const auto& model_name : all_block_model_names_) {
            if (model_name == edit_model_name) continue;
            if (ImGui::Selectable(model_name.data())) {
              model_data = BlockDB::LoadBlockModelDataFromName(model_name);
              if (!model_data.has_value()) {
                spdlog::error("Unable to load model data: {}", model_name);
                break;
              }
              edit_model_name = model_name;
              set_data(model_data);
              HandleEditModelChange();
            }
          }
          ImGui::EndCombo();
        }

        ImGui::Text("Textures");
        for (uint32_t i = 0; i < 3; i++) {
          ImGui::BeginDisabled(static_cast<uint32_t>(edit_model_type_) == i);
          if (ImGui::Button(TexTypeNames[i].data())) {
            edit_model_type_ = static_cast<BlockModelType>(i);
            HandleEditModelChange();
          }
          ImGui::EndDisabled();
          if (i < 2) ImGui::SameLine();
        }

        TexSelectMenu(edit_mode_);

        if (ImGui::Button("Reset")) {
          edit_model_data_all_ = original_edit_model_data_all;
          edit_model_data_top_bot_ = original_edit_model_top_bot;
          edit_model_data_unique_ = original_edit_model_unique;
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
          std::string path = GET_PATH("resources/data/model/" + edit_model_name + ".json");
          if (edit_model_type_ == BlockModelType::All) {
            BlockDB::WriteBlockModelTypeAll(edit_model_data_all_, path);
          } else if (edit_model_type_ == BlockModelType::TopBottom) {
            BlockDB::WriteBlockModelTypeTopBot(edit_model_data_top_bot_, path);
          } else {
            BlockDB::WriteBlockModelTypeUnique(edit_model_data_unique_, path);
          }
        }
        ImGui::EndTabItem();
      }
    }
    if (ImGui::BeginTabItem("Edit Block")) {
      ZoneScopedN("Edit Block tab");
      edit_mode_ = EditMode::EditBlock;
      auto& block_data_arr = block_db_.block_data_arr_;
      auto& block_data_model_names = block_db_.block_model_names_;

      if (ImGui::BeginCombo("Block##edit_block_select",
                            block_data_arr[edit_block_idx_].name.c_str())) {
        for (uint32_t i = 1; i < block_data_arr.size(); i++) {
          const auto& block_data = block_data_arr[i];
          if (block_data.name == block_data_arr[edit_block_idx_].name) continue;
          if (ImGui::Selectable(block_data.name.data())) {
            original_edit_block_data_ = block_data;
            edit_block_idx_ = i;
            player_.SetPosition(glm::vec3{0.f, 0.5f, 5.0f} + blocks_[i].pos);
            player_.GetCamera().LookAt(blocks_[i].pos);
          }
        }
        ImGui::EndCombo();
      }

      ImGui::InputText("Name", &block_data_arr[edit_block_idx_].name);
      ImGui::Checkbox("Emits Light", &block_data_arr[edit_block_idx_].emits_light);
      if (ImGui::BeginCombo("##Model",
                            ("Model: " + block_data_model_names[edit_block_idx_]).c_str())) {
        for (const auto& model : all_block_model_names_) {
          if (model == block_data_model_names[edit_block_idx_]) continue;
          if (ImGui::Selectable(model.data())) {
            block_data_model_names[edit_block_idx_] = model;
          }
        }
        ImGui::EndCombo();
      }

      bool changes_exist = original_edit_block_data_ != block_data_arr[edit_block_idx_];
      ImGui::BeginDisabled(!changes_exist);
      if (ImGui::Button("Cancel")) {
        block_data_arr[edit_block_idx_] = original_edit_block_data_;
        block_data_model_names[edit_block_idx_] = original_edit_block_model_name_;
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      ImGui::BeginDisabled(!changes_exist);
      if (ImGui::Button("Save")) {
        block_db_.WriteBlockData(block_data_arr[edit_block_idx_],
                                 block_data_model_names[edit_block_idx_]);
        original_edit_block_data_ = block_data_arr[edit_block_idx_];
        original_edit_block_model_name_ = block_data_model_names[edit_block_idx_];
      }
      ImGui::EndDisabled();
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Add Block")) {
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
