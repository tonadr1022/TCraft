#include "WorldScene.hpp"

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
#include "renderer/opengl/Texture2d.hpp"
#include "resource/Image.hpp"
#include "resource/MaterialManager.hpp"
#include "resource/TextureManager.hpp"
#include "util/JsonUtil.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

WorldScene::WorldScene(SceneManager& scene_manager, std::string_view path)
    : Scene(scene_manager),
      chunk_manager_(std::make_unique<ChunkManager>(block_db_)),
      player_(*chunk_manager_, block_db_),
      directory_path_(path) {
  ZoneScoped;
  EASSERT_MSG(!path.empty(), "Can't load world scene without a loaded world name");
  {
    ZoneScopedN("Initialize data");
    try {
      auto data = util::LoadJsonFile(std::string(path) + "/data.json");
      auto level_data = util::LoadJsonFile(std::string(path) + "/level.json");
      auto seed = util::json::GetString(level_data, "seed");
      EASSERT_MSG(seed.has_value(), "Missing seed from level.json");
      static std::hash<std::string> hasher;
      seed_ = hasher(seed.value());
      chunk_manager_->SetSeed(seed_);
      std::array<float, 3> player_pos =
          data.value("player_position", std::array<float, 3>{0, 0, 0});
      player_.SetPosition({player_pos[0], player_pos[1], player_pos[2]});
      auto camera = data["camera"];
      float pitch = 0, yaw = 0;
      if (camera.is_object()) {
        pitch = camera.value("pitch", 0);
        yaw = camera.value("yaw", 0);
      }
      player_.GetFPSCamera().SetOrientation(pitch, yaw);
    } catch (std::runtime_error& error) {
      spdlog::info("failed to load world data");
      scene_manager_.SetNextSceneOnConstructionError("main_menu");
      return;
    }
  }
  {
    ZoneScopedN("Texture load");
    cross_hair_mat_ =
        MaterialManager::Get().LoadTextureMaterial({.filepath = GET_TEXTURE_PATH("crosshair.png")});
  }

  {
    ZoneScopedN("Load block mesh data");
    std::unordered_map<std::string, uint32_t> tex_name_to_idx;
    Image image;
    int tex_idx = 0;
    std::vector<void*> all_texture_pixel_data;
    std::vector<Image> images;
    for (const auto& tex_name : block_db_.GetTextureNamesInUse()) {
      util::LoadImage(image, GET_PATH("resources/textures/" + tex_name + ".png"));
      // TODO: handle other sizes/animations
      if (image.width != 32 || image.height != 32) continue;
      tex_name_to_idx[tex_name] = tex_idx++;
      all_texture_pixel_data.emplace_back(image.pixels);
      images.emplace_back(image);
    }
    chunk_render_params_.chunk_tex_array_handle =
        TextureManager::Get().Create2dArray({.all_pixels_data = all_texture_pixel_data,
                                             .dims = glm::ivec2{32, 32},
                                             .generate_mipmaps = true,
                                             .internal_format = GL_RGBA8,
                                             .format = GL_RGBA,
                                             .filter_mode_min = GL_NEAREST_MIPMAP_LINEAR,
                                             .filter_mode_max = GL_NEAREST,
                                             .texture_wrap = GL_REPEAT});

    block_db_.LoadMeshData(tex_name_to_idx, images);
    for (auto* const p : all_texture_pixel_data) {
      util::FreeImage(p);
    }
  }
  chunk_manager_->Init(player_.Position());
}

void WorldScene::Update(double dt) {
  ZoneScoped;
  chunk_manager_->SetCenter(player_.Position());
  chunk_manager_->Update(dt);
  loaded_ = loaded_ || chunk_manager_->IsLoaded();
  // if (loaded_) player_.Update(dt);
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
      chunk_map_mode_ = static_cast<ChunkManager::ChunkMapMode>(
          (static_cast<int>(chunk_map_mode_) + 1) %
          (static_cast<int>(ChunkManager::ChunkMapMode::Count)));
    }
  }
  return false;
}

void WorldScene::Render() {
  ZoneScoped;
  glm::mat4 proj = player_.GetCamera().GetProjection(window_.GetAspectRatio());
  glm::mat4 view = player_.GetCamera().GetView();
  RenderInfo render_info{.vp_matrix = proj * view,
                         .view_matrix = view,
                         .proj_matrix = proj,
                         .view_pos = player_.Position()};

  auto win_center = window_.GetWindowCenter();

  if (loaded_) {
    Renderer::Get().DrawQuad(cross_hair_mat_->Handle(), {win_center.x, win_center.y}, {20, 20});
    auto ray_cast_pos = player_.GetRayCastBlockPos();
    if (ray_cast_pos != glm::NullIVec3) {
      Renderer::Get().DrawBlockOutline(ray_cast_pos, view, proj);
    }
  } else {
    // draw loading bar
    const auto& state = chunk_manager_->GetStateStats();
    float loading_percentage =
        static_cast<float>(state.loaded_chunks) / static_cast<float>(state.max_chunks);
    for (int i = 0; i < 100; i++) {
      auto color = static_cast<float>(i) / 100.f >= loading_percentage ? color::Red : color::Green;
      Renderer::Get().DrawQuad(color, glm::ivec2{win_center.x + i * 3, win_center.y + 275},
                               {10, 10});
    }
  }

  {
    ZoneScopedN("Chunk state render");
    glm::ivec2 dims;
    chunk_manager_->PopulateChunkStatePixels(chunk_state_pixels_, dims, state_pix_y_, .5,
                                             chunk_map_mode_);
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

  chunk_render_params_.render_chunks_ = loaded_;
  Renderer::Get().RenderWorld(chunk_render_params_, render_info);
}

WorldScene::~WorldScene() {
  ZoneScoped;
  auto pos = player_.Position();
  // TODO: save world function?
  std::array<float, 3> player_pos = {pos.x, pos.y, pos.z};
  // TODO: player save function
  nlohmann::json j = {
      {"player_position", player_pos},
      {"camera",
       {{"pitch", player_.GetCamera().GetPitch()}, {"yaw", player_.GetCamera().GetYaw()}}}};
  util::json::WriteJson(j, directory_path_ + "/data.json");
}

void WorldScene::OnImGui() {
  ZoneScoped;
  chunk_manager_->OnImGui();

  player_.OnImGui();
  ImGui::Text("time: %f", time_);
  ImGui::SliderInt("Chunk State Y", &state_pix_y_, 0, NumVerticalChunks);
}
