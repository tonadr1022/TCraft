#include "WorldScene.hpp"

#include <imgui.h>

#include <iostream>
#include <nlohmann/json.hpp>

#include "application/SceneManager.hpp"
#include "application/SettingsManager.hpp"
#include "application/Window.hpp"
#include "gameplay/world/Block.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/TerrainGenerator.hpp"
#include "pch.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Renderer.hpp"
#include "resource/TextureManager.hpp"
#include "util/Paths.hpp"

WorldScene::WorldScene(SceneManager& scene_manager, const std::string& world_name)
    : Scene(scene_manager) {
  ZoneScoped;
  EASSERT_MSG(!world_name.empty(), "Can't load world scene without a loaded world name");
  name_ = "World Scene";

  std::unordered_map<std::string, uint32_t> name_to_idx;
  world_render_params_.chunk_tex_array_handle = TextureManager::Get().Create2dArray(
      GET_PATH("resources/data/block/texture_2d_array.json"), name_to_idx);
  block_db_ = std::make_unique<BlockDB>(name_to_idx);

  player_.GetCamera().Load();
  player_.Init();
  auto settings = SettingsManager::Get().LoadSetting("world");
  load_distance_ = settings["load_distance"].get<int>();

  std::array<float, 3> player_pos = settings["player_position"].get<std::array<float, 3>>();
  player_.position_ = {player_pos[0], player_pos[1], player_pos[2]};

  // TODO(tony): make spiral
  // glm::ivec3 iter;
  // for (iter.z = -load_distance_; iter.z <= load_distance_; iter.z++) {
  //   for (iter.y = -load_distance_; iter.y <= load_distance_; iter.y++) {
  //     for (iter.x = -load_distance_; iter.x <= load_distance_; iter.x++) {
  //     }
  //   }
  // }

  glm::ivec3 pos{0, 0, 0};
  chunk_map_.emplace(pos, std::make_unique<Chunk>());
  Chunk* chunk = chunk_map_.at(pos).get();
  std::vector<BlockType> blocks = {BlockType::Dirt, BlockType::DiamondOre};
  TerrainGenerator::GenerateChecker(chunk->data, blocks);
  ChunkMesher mesher{*block_db_};
  mesher.GenerateNaive(chunk->data, chunk->mesh.vertices, chunk->mesh.indices);
  chunk->mesh.Allocate();
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
  RenderInfo render_info;
  render_info.window_dims = window.GetWindowSize();
  float aspect_ratio =
      static_cast<float>(render_info.window_dims.x) / static_cast<float>(render_info.window_dims.y);
  render_info.vp_matrix =
      player_.GetCamera().GetProjection(aspect_ratio) * player_.GetCamera().GetView();
  renderer.RenderWorld(*this, render_info, *block_db_);
}

WorldScene::~WorldScene() {
  ZoneScoped;
  player_.GetCamera().Save();
  std::array<float, 3> player_pos = {player_.position_.x, player_.position_.y, player_.position_.z};
  nlohmann::json j = {{"load_distance", load_distance_}, {"player_position", player_pos}};
  SettingsManager::Get().SaveSetting(j, "world");
}

void WorldScene::OnImGui() {
  ZoneScoped;
  player_.OnImGui();
  if (ImGui::CollapsingHeader("World", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::SliderInt("Load Distance", &load_distance_, 1, 32);
  }
}
