#include "WorldScene.hpp"

#include <imgui.h>

#include <nlohmann/json.hpp>

#include "Constants.hpp"
#include "application/SceneManager.hpp"
#include "application/Window.hpp"
#include "gameplay/GamePlayer.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkManager.hpp"
#include "renderer/Renderer.hpp"
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
      std::array<float, 3> player_pos =
          data.value("player_position", std::array<float, 3>{0, 0, 0});
      player_.SetPosition({player_pos[0], player_pos[1], player_pos[2]});
      auto camera = data["camera"];
      float pitch = camera.value("pitch", 0);
      float yaw = camera.value("yaw", 0);
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

  player_.Init();
  block_db_.Init();
  {
    ZoneScopedN("Load block mesh data");
    std::unordered_map<std::string, uint32_t> tex_name_to_idx;
    Image image;
    int tex_idx = 0;
    std::vector<void*> all_texture_pixel_data;
    for (const auto& tex_name : block_db_.GetTextureNamesInUse()) {
      util::LoadImage(image, GET_PATH("resources/textures/" + tex_name + ".png"));
      // TODO: handle other sizes/animations
      if (image.width != 32 || image.height != 32) continue;
      tex_name_to_idx[tex_name] = tex_idx++;
      all_texture_pixel_data.emplace_back(image.pixels);
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

    block_db_.LoadMeshData(tex_name_to_idx);
  }
  chunk_manager_->Init();
}

void WorldScene::Update(double dt) {
  ZoneScoped;
  chunk_manager_->SetCenter(player_.Position());
  chunk_manager_->Update(dt);
  player_.Update(dt);
}

bool WorldScene::OnEvent(const SDL_Event& event) {
  if (player_.OnEvent(event)) {
    return true;
  }
  switch (event.type) {
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_p && event.key.keysym.mod & KMOD_ALT) {
        scene_manager_.LoadScene("main_menu");
        return true;
      }
      if (event.key.keysym.sym == SDLK_r && event.key.keysym.mod & KMOD_CTRL &&
          event.key.keysym.mod & KMOD_SHIFT) {
        chunk_manager_ = std::make_unique<ChunkManager>(block_db_);
        chunk_manager_->Init();
      }
  }
  return false;
}

void WorldScene::Render() {
  ZoneScoped;
  RenderInfo render_info{.vp_matrix = player_.GetCamera().GetProjection(window_.GetAspectRatio()) *
                                      player_.GetCamera().GetView()};
  auto win_center = window_.GetWindowCenter();
  Renderer::Get().DrawQuad(cross_hair_mat_->Handle(), {win_center.x, win_center.y}, {20, 20});
  {
    ZoneScopedN("Submit chunk draw commands");
    // TODO: only send to renderer the chunks ready to be rendered instead of the whole map
    for (const auto& it : chunk_manager_->GetVisibleChunks()) {
      if (!it.second.mesh.IsAllocated()) continue;
      glm::vec3 pos = it.first * ChunkLength;
      Renderer::Get().SubmitChunkDrawCommand(glm::translate(glm::mat4{1}, pos),
                                             it.second.mesh.Handle());
    }
  }

  auto ray_cast_pos = player_.GetRayCastBlockPos();
  if (ray_cast_pos != glm::NullIVec3) {
    Renderer::Get().DrawBlockOutline(
        ray_cast_pos, player_.GetCamera().GetView(),
        player_.GetCamera().GetProjection(Window::Get().GetAspectRatio()));
  }

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
}
