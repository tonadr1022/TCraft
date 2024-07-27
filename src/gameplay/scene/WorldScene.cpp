#include "WorldScene.hpp"

#include <imgui.h>

#include <nlohmann/json.hpp>

#include "application/SceneManager.hpp"
#include "application/SettingsManager.hpp"
#include "application/Window.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkManager.hpp"
#include "renderer/Renderer.hpp"
#include "resource/TextureManager.hpp"
#include "util/Paths.hpp"

WorldScene::WorldScene(SceneManager& scene_manager, const std::string& world_name)
    : Scene(scene_manager), chunk_manager_(block_db_, scene_manager.GetRenderer()) {
  ZoneScoped;
  EASSERT_MSG(!world_name.empty(), "Can't load world scene without a loaded world name");

  std::unordered_map<std::string, uint32_t> tex_name_to_idx;
  world_render_params_.chunk_tex_array_handle = TextureManager::Get().Create2dArray(
      GET_PATH("resources/data/block/texture_2d_array.json"), tex_name_to_idx);
  block_db_.Init(tex_name_to_idx, false);
  chunk_manager_.Init();

  player_.GetCamera().Load();
  player_.Init();
  auto settings = SettingsManager::Get().LoadSetting("world");

  std::array<float, 3> player_pos = settings["player_position"].get<std::array<float, 3>>();
  player_.position_ = {player_pos[0], player_pos[1], player_pos[2]};
}

void WorldScene::Update(double dt) {
  ZoneScoped;
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
  }
  return false;
}

void WorldScene::Render(Renderer& renderer, const Window& window) {
  ZoneScoped;
  RenderInfo render_info{.vp_matrix = player_.GetCamera().GetProjection(window.GetAspectRatio()) *
                                      player_.GetCamera().GetView()};
  renderer.RenderWorld(*this, render_info);
}

WorldScene::~WorldScene() {
  ZoneScoped;
  player_.GetCamera().Save();
  std::array<float, 3> player_pos = {player_.position_.x, player_.position_.y, player_.position_.z};
  nlohmann::json j = {{"player_position", player_pos}};
  SettingsManager::Get().SaveSetting(j, "world");
}

void WorldScene::OnImGui() {
  ZoneScoped;
  chunk_manager_.OnImGui();
  player_.OnImGui();
}
