#include "WorldScene.hpp"

#include <SDL_timer.h>
#include <imgui.h>

#include <nlohmann/json.hpp>

#include "Constants.hpp"
#include "application/SceneManager.hpp"
#include "application/Window.hpp"
#include "gameplay/GamePlayer.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkManager.hpp"
#include "renderer/Constants.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/RendererUtil.hpp"
#include "renderer/ShaderManager.hpp"
#include "renderer/Shape.hpp"
#include "renderer/opengl/Texture2d.hpp"
#include "resource/Image.hpp"
#include "resource/MaterialManager.hpp"
#include "resource/TextureManager.hpp"
#include "util/JsonUtil.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

WorldScene::WorldScene(SceneManager& scene_manager, const std::string& directory_path)
    : Scene(scene_manager),
      chunk_manager_(std::make_unique<ChunkManager>(block_db_)),
      player_(*chunk_manager_, block_db_),
      directory_path_(directory_path) {
  ZoneScoped;
  std::filesystem::path data_path = directory_path / std::filesystem::path("data.json");
  std::filesystem::path level_path = directory_path / std::filesystem::path("level.json");
  if (!std::filesystem::exists(data_path)) {
    spdlog::error("Required file does not exist: {}", data_path.string());
    return;
  }
  if (!std::filesystem::exists(level_path)) {
    spdlog::error("Required file does not exist: {}", level_path.string());
    return;
  }

  auto data = util::LoadJsonFile(data_path);
  auto level_data = util::LoadJsonFile(level_path);
  auto seed = util::json::GetString(level_data, "seed");
  EASSERT_MSG(seed.has_value(), "Missing seed from level.json");
  static std::hash<std::string> hasher;
  seed_ = hasher(seed.value());
  chunk_manager_->SetSeed(seed_);
  std::array<float, 3> player_pos = data.value("player_position", std::array<float, 3>{0, 0, 0});
  player_.SetPosition({player_pos[0], player_pos[1], player_pos[2]});
  auto camera = data["camera"];
  float pitch = 0, yaw = 0;
  if (camera.is_object()) {
    pitch = camera.value("pitch", 0);
    yaw = camera.value("yaw", 0);
  }
  player_.SetMovementSpeed(data.value("player_movement_speed", 10.f));
  player_.GetFPSCamera().SetOrientation(pitch, yaw);

  cross_hair_mat_ =
      MaterialManager::Get().LoadTextureMaterial({.filepath = GET_TEXTURE_PATH("crosshair.png")});

  {
    ZoneScopedN("Load block mesh data");
    std::unordered_map<std::string, uint32_t> tex_name_to_idx;
    Image image;
    int tex_idx = 0;
    std::vector<Image> images;
    for (const auto& tex_name : block_db_.GetTextureNamesInUse()) {
      util::LoadImage(image, GET_PATH("resources/textures/" + tex_name + ".png"), 4, true);
      // TODO: handle other sizes/animations
      if (image.width != 32 || image.height != 32) continue;
      tex_name_to_idx[tex_name] = tex_idx++;
      images.emplace_back(image);
    }
    chunk_tex_array_ = TextureManager::Get().Load({.images = images,
                                                   .generate_mipmaps = true,
                                                   .internal_format = GL_RGBA8,
                                                   .format = GL_RGBA,
                                                   .filter_mode_min = GL_NEAREST_MIPMAP_LINEAR,
                                                   .filter_mode_max = GL_NEAREST,
                                                   .texture_wrap = GL_REPEAT});
    Renderer::Get().chunk_tex_array = chunk_tex_array_;
    block_db_.LoadMeshData(tex_name_to_idx, images);
    for (auto const& p : images) {
      util::FreeImage(p.pixels);
    }
  }

  std::vector<Vertex> cube_vertices;
  for (size_t i = 0; i < kCubeVertices.size(); i += 5) {
    cube_vertices.emplace_back(
        glm::vec3{kCubeVertices[i], kCubeVertices[i + 1], kCubeVertices[i + 2]},
        glm::vec2{kCubeVertices[i + 3], kCubeVertices[i + 4]});
  }
  std::vector cube_indices(kCubeIndices.begin(), kCubeIndices.end());
  cube_mesh_.Allocate(cube_vertices, cube_indices);

  Renderer::Get().SetSkyboxShaderFunc([this]() {
    auto skybox_shader = ShaderManager::Get().GetShader("skybox").value();
    skybox_shader.Bind();
    skybox_shader.SetMat4("vp_matrix", curr_render_info_.proj_matrix *
                                           glm::mat4(glm::mat3(curr_render_info_.view_matrix)));

    double curr_time = SDL_GetPerformanceCounter() * .000000001;
    if (animate_time_of_day_) {
      if (sun_movement_mode_ == SunMovementMode::kRegular) {
        sun_dir_ = glm::normalize(glm::vec3{glm::cos(curr_time), glm::sin(curr_time), 0});
      } else if (sun_movement_mode_ == SunMovementMode::kCircleDaytime) {
        sun_dir_ = glm::normalize(
            glm::vec3{glm::cos(curr_time), sun_circle_animate_height_, glm::sin(curr_time)});
      }
    }
    skybox_shader.SetVec3("u_sun_direction", sun_dir_);
    skybox_shader.SetFloat("u_time", curr_time * 0.5);
  });

  chunk_manager_->Init(player_.Position());

  icon_texture_atlas_ =
      util::renderer::LoadIconTextureAtlas("world_scene_icons", block_db_, *chunk_tex_array_);

  player_.held_item_id = block_db_.GetBlockData("stone")->id;
}

void WorldScene::Update(double dt) {
  ZoneScoped;
  chunk_manager_->SetCenter(player_.Position());
  chunk_manager_->Update(dt);
  loaded_ = loaded_ || chunk_manager_->IsLoaded();
  if (!loaded_) time_ += dt;
  player_.Update(dt);
}

bool WorldScene::OnEvent(const SDL_Event& event) {
  if (player_.OnEvent(event)) {
    return true;
  }
  if (!loaded_) return false;
  if (event.type == SDL_KEYDOWN) {
    if (event.key.keysym.sym == SDLK_p && event.key.keysym.mod & KMOD_ALT) {
      scene_manager_.LoadScene("main_menu");
      return true;
    }
    if (event.key.keysym.sym == SDLK_r && event.key.keysym.mod & KMOD_CTRL &&
        event.key.keysym.mod & KMOD_SHIFT) {
      chunk_manager_ = std::make_unique<ChunkManager>(block_db_);
      chunk_manager_->SetSeed(seed_);
      return true;
    }
  } else if (event.type == SDL_MOUSEBUTTONDOWN) {
    auto pos = Window::Get().GetMousePosition();
    auto dims = Window::Get().GetWindowSize();
    if (pos.x < dims.x / 2 && dims.y - pos.y < dims.y / 2) {
      chunk_map_mode_ = static_cast<ChunkMapMode>((static_cast<int>(chunk_map_mode_) + 1) %
                                                  (static_cast<int>(ChunkMapMode::kCount)));
    }
  }
  return false;
}

void WorldScene::Render() {
  ZoneScoped;
  auto& camera = player_.GetCamera();
  glm::mat4 proj = camera.GetProjection(window_.GetAspectRatio());
  glm::mat4 view = camera.GetView();
  curr_render_info_ = {.vp_matrix = proj * view,
                       .view_matrix = view,
                       .proj_matrix = proj,
                       .view_pos = player_.Position(),
                       .view_dir = camera.GetFront(),
                       .camera_near_plane = camera.NearPlane(),
                       .camera_far_plane = camera.FarPlane()};
  auto win_center = window_.GetWindowCenter();

  if (loaded_) {
    Renderer::Get().DrawQuad(cross_hair_mat_->Handle(), {win_center.x, win_center.y}, {20, 20});
    auto ray_cast_pos = player_.GetRayCastBlockPos();
    if (ray_cast_pos != glm::kNullIVec3) {
      Renderer::Get().DrawLine(glm::translate(glm::mat4{1}, glm::vec3(ray_cast_pos)), glm::vec3(0),
                               cube_mesh_.Handle(), false);
    }
  }

  if (show_chunk_map_) {
    ZoneScopedN("Chunk state render");
    glm::ivec2 dims;
    chunk_manager_->PopulateChunkStatePixels(chunk_state_pixels_, dims, chunk_map_display_y_level_,
                                             .5, chunk_map_mode_);
    if (dims != prev_chunk_state_pixels_dims_ || first_frame_) {
      first_frame_ = false;
      MaterialManager::Get().Erase("chunk_state_map");

      chunk_state_tex_ = MaterialManager::Get().LoadTextureMaterial(
          "chunk_state_map", Texture2DCreateParamsEmpty{.width = static_cast<uint32_t>(dims.x),
                                                        .height = static_cast<uint32_t>(dims.y),
                                                        .bindless = true});
    }
    glTextureSubImage2D(chunk_state_tex_->GetTexture().Id(), 0, 0, 0, dims.x, dims.y, GL_RGBA,
                        GL_UNSIGNED_BYTE, chunk_state_pixels_.data());
    prev_chunk_state_pixels_dims_ = dims;

    glm::ivec2 quad_dims = loaded_ ? glm::ivec2{250, 250} : glm::ivec2{500, 500};
    glm::ivec2 quad_pos = loaded_ ? quad_dims / 2 : win_center;
    Renderer::Get().DrawQuad(chunk_state_tex_->Handle(), quad_pos, quad_dims);
  }

  Renderer::Get().directional_light_dir = sun_dir_;
  Renderer::Get().Render(curr_render_info_);
}

WorldScene::~WorldScene() {
  ZoneScoped;
  auto pos = player_.Position();
  // TODO: save world function?
  std::array<float, 3> player_pos = {pos.x, pos.y, pos.z};
  // TODO: player save function
  nlohmann::json j = {
      {"player_position", player_pos},
      {"player_movement_speed", player_.GetMovementSpeed()},
      {"camera",
       {{"pitch", player_.GetCamera().GetPitch()}, {"yaw", player_.GetCamera().GetYaw()}}}};
  util::json::WriteJson(j, directory_path_ + "/data.json");
  Renderer::Get().chunk_tex_array = nullptr;
}

void WorldScene::OnImGui() {
  ZoneScoped;

  chunk_manager_->OnImGui();
  if (ImGui::Button("Reload Icons")) {
    util::renderer::RenderAndWriteIcons(block_db_.GetBlockData(), block_db_.GetMeshData(),
                                        *chunk_tex_array_, false);
    TextureManager::Get().Erase("world_scene_icons");
    icon_texture_atlas_ =
        util::renderer::LoadIconTextureAtlas("world_scene_icons", block_db_, *chunk_tex_array_);
  }
  player_.OnImGui();
  ImGui::Text("time: %f", time_);
  ImGui::SliderInt("Chunk State Y", &chunk_map_display_y_level_, 0, kNumVerticalChunks);
  ImGui::Checkbox("Show Chunk Map", &show_chunk_map_);
  ImGui::Checkbox("Animate Time of Day", &animate_time_of_day_);
  int mode = static_cast<int>(sun_movement_mode_);
  ImGui::SliderFloat("Sun Animation Circle Height", &sun_circle_animate_height_, 0.f, 10.f);
  if (ImGui::SliderInt("Sun Animation Mode", &mode, 0, 1)) {
    sun_movement_mode_ = static_cast<SunMovementMode>(mode);
  }
  ImGui::SliderFloat3("Dir light dir", &sun_dir_.x, -1, 1);
  DrawInventory();
}

void WorldScene::DrawInventory() {
  ImGuiStyle& style = ImGui::GetStyle();
  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;
  bool inventory_open{true};
  glm::vec2 img_dims{50, 50};
  // static int icons_per_row = 6;
  float max_inv_screen_width_percentage = 1.f / 3.f;
  auto full_window_dims = Window::Get().GetWindowSize();
  float max_inv_width = full_window_dims.x * max_inv_screen_width_percentage;

  float padded_width = img_dims.x + style.ItemSpacing.x;
  int icons_per_row = std::min(6, (static_cast<int>(max_inv_width / padded_width)));
  uint32_t num_rows =
      glm::ceil(static_cast<float>(icon_texture_atlas_.id_to_offset_map.size()) / icons_per_row);
  float window_width =
      (img_dims.x * icons_per_row + style.ItemSpacing.x * (icons_per_row - 1)) * 1.25 +
      style.WindowPadding.x * 2;
  float window_height = img_dims.y * num_rows * 1.15 + style.ItemSpacing.y * (num_rows - 1) +
                        style.WindowPadding.y * 2;
  ImGui::SliderFloat2("window padding", &style.WindowPadding.x, 0.0f, 10.0f);
  ImGui::SliderFloat2("item spacing", &style.ItemSpacing.x, 0.0f, 10.0f);
  ImGui::Text("Held item id %i", player_.held_item_id);
  ImGui::SetNextWindowPos(ImVec2(full_window_dims.x - window_width, 0));
  ImGui::SetNextWindowSize(ImVec2(window_width, window_height));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.3f));
  ImGui::Begin("Inventory", &inventory_open, flags);
  int i = 0;
  ImVec2 uv0, uv1;
  for (size_t id = 1; id < block_db_.GetBlockData().size(); id++) {
    icon_texture_atlas_.ComputeUVs(id, uv0, uv1);

    bool is_selected = (id == static_cast<size_t>(player_.held_item_id));
    if (is_selected) {
      constexpr ImVec4 kActiveColor(0.2, 0.8f, 0.2f, 0.5f);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(0.0f, 0.0f, 0.6f, 0.0f));  // Default button color
      ImGui::PushStyleColor(ImGuiCol_Button,
                            kActiveColor);  // Change button color when selected
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kActiveColor);  // Hover color
    } else {
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(0.0f, 0.0f, 0.6f, 0.0f));  // Default button color
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImVec4(0.6f, 0.6f, 0.6f, 0.0f));  // Default button color
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.7f, 0.7f, 0.5f));  // Hover color
    }

    ImGui::PushID(id);
    if (ImGui::ImageButton(reinterpret_cast<void*>(icon_texture_atlas_.material->GetTexture().Id()),
                           ImVec2(img_dims.x, img_dims.y), uv0, uv1)) {
      player_.held_item_id = id;
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
      ImGui::SetTooltip("%s", block_db_.GetBlockData()[id].formatted_name.c_str());
    }
    ImGui::PopID();

    ImGui::PopStyleColor(3);

    if (++i % icons_per_row != 0) {
      ImGui::SameLine();
    }
  }
  ImGui::End();
  ImGui::PopStyleColor();
}
