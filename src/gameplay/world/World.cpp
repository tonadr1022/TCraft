#include "World.hpp"

#include <imgui.h>

#include <glm/vec3.hpp>
#include <nlohmann/json_fwd.hpp>

#include "application/Settings.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/world/TerrainGenerator.hpp"
#include "renderer/ChunkMesher.hpp"

void World::Update(double dt) {
  ZoneScoped;
  player_.Update(dt);
}

void World::Load(std::string_view /*path*/) {
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

bool World::OnEvent(const SDL_Event& event) { return player_.OnEvent(event); }

void World::Save() {
  player_.GetCamera().Save();
  std::array<float, 3> player_pos = {player_.position_.x, player_.position_.y, player_.position_.z};
  nlohmann::json j = {{"load_distance", load_distance_}, {"player_position", player_pos}};
  Settings::Get().SaveSetting(j, "world");
}

void World::OnImGui() {
  player_.OnImGui();
  if (ImGui::CollapsingHeader("World", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::SliderInt("Load Distance", &load_distance_, 1, 32);
  }
}
