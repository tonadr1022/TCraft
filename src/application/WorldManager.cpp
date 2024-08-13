#include "WorldManager.hpp"

#include <nlohmann/json.hpp>

#include "util/JsonUtil.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

namespace fs = std::filesystem;

void WorldManager::CreateWorld(const WorldCreateParams& params) {
  if (!fs::exists(GET_PATH("resources/worlds"))) {
    fs::create_directory(GET_PATH("resources/worlds"));
  }
  fs::path world_path = fs::path(GET_PATH("resources/worlds")) / params.name;
  fs::path level_path = world_path / "level.json";
  EASSERT_MSG(!fs::exists(level_path), "Create World called without ensuring world doesn't exist");
  fs::remove_all(world_path);
  fs::create_directory(world_path);
  // TODO: either better id or get rid of it
  nlohmann::json level_data = {{"name", params.name}, {"id", rand()}, {"seed", params.seed}};
  util::json::WriteJson(level_data, level_path);
  nlohmann::json settings = {{"camera", {{"pitch", 0.0f}, {"yaw", 0.0f}}},
                             {"player_position", std::array<float, 3>{0.0f, 0.0f, 0.0f}}};

  util::json::WriteJson(settings, world_path / "data.json");
}

void WorldManager::DeleteWorld(std::string_view) {}

WorldManager::WorldManager() {
  for (const auto& dir : fs::directory_iterator(GET_PATH("resources/worlds"))) {
    const fs::path level_path = dir.path() / "level.json";
    if (fs::exists(level_path)) {
      auto obj = util::LoadJsonFile(level_path.string());
      if (obj.is_object() && obj.contains("name") && obj.contains("id")) {
        world_names_.insert(dir.path().filename());
      }
    }
  }
}

bool WorldManager::WorldExists(const std::string& name) { return world_names_.contains(name); }
