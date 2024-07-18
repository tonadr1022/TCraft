#include "WorldScene.hpp"

#include <imgui.h>

#include <memory>
#include <nlohmann/json.hpp>

#include "application/Settings.hpp"
#include "application/Window.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/TerrainGenerator.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Renderer.hpp"

WorldScene::WorldScene(SceneManager& scene_manager)
    : Scene(scene_manager), block_db_(std::make_unique<BlockDB>()) {
  ZoneScoped;
  player_.GetCamera().Load();
  player_.Init();
  auto settings = Settings::Get().LoadSetting("world");
  load_distance_ = settings["load_distance"].get<int>();

  std::array<float, 3> player_pos = settings["player_position"].get<std::array<float, 3>>();
  player_.position_ = {player_pos[0], player_pos[1], player_pos[2]};

  // TODO(tony): load from files

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
  TerrainGenerator::GenerateChecker(chunk->data, BlockType::Dirt);
  ChunkMesher::GenerateNaive(chunk->data, chunk->mesh.vertices, chunk->mesh.indices);
  chunk->mesh.Allocate();
}

void WorldScene::Update(double dt) {
  ZoneScoped;
  player_.Update(dt);
}

bool WorldScene::OnEvent(const SDL_Event& event) { return player_.OnEvent(event); }

void WorldScene::Render(Renderer& renderer, const Window& window) {
  ZoneScoped;
  RenderInfo render_info;
  render_info.window_dims = window.GetWindowSize();
  float aspect_ratio =
      static_cast<float>(render_info.window_dims.x) / static_cast<float>(render_info.window_dims.y);
  render_info.vp_matrix =
      player_.GetCamera().GetProjection(aspect_ratio) * player_.GetCamera().GetView();
  renderer.RenderWorld(*this, render_info);
}

WorldScene::~WorldScene() {
  ZoneScoped;
  player_.GetCamera().Save();
  std::array<float, 3> player_pos = {player_.position_.x, player_.position_.y, player_.position_.z};
  nlohmann::json j = {{"load_distance", load_distance_}, {"player_position", player_pos}};
  Settings::Get().SaveSetting(j, "world");
}

void WorldScene::OnImGui() {
  ZoneScoped;
  player_.OnImGui();
  if (ImGui::CollapsingHeader("World", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::SliderInt("Load Distance", &load_distance_, 1, 32);
  }
}
