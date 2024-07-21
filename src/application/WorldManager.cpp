#include "WorldManager.hpp"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "util/JsonUtil.hpp"
#include "util/LoadFile.hpp"

namespace fs = std::filesystem;

void WorldManager::CreateWorld(const WorldCreateParams& params) {
  if (!fs::exists("resources/worlds")) {
    fs::create_directory("resources/worlds");
  }
  fs::path world_path = fs::path("resources/worlds") / params.name;
  fs::path level_path = world_path / "level.json";
  EASSERT_MSG(!fs::exists(level_path), "Create World called without ensuring world doesn't exist");
  fs::remove_all(world_path);
  fs::create_directory(world_path);
  nlohmann::json level_data = {{"name", params.name}, {"id", rand()}};
  json_util::WriteJson(level_data, level_path);
  nlohmann::json settings = {};
  json_util::WriteJson(settings, world_path / "settings.json");
}

void WorldManager::DeleteWorld(std::string_view name) {}

WorldManager::WorldManager() {
  for (const auto& dir : fs::directory_iterator("resources/worlds")) {
    const fs::path& dir_path = dir.path();
    const fs::path level_path = dir_path / "level.json";
    if (fs::exists(level_path)) {
      spdlog::info("{}", SRC_PATH + level_path.string());
      auto obj = util::LoadJsonFile(SRC_PATH + level_path.string());
      if (obj.is_object() && obj.contains("name") && obj.contains("id")) {
        spdlog::info("{}", dir_path.filename().string());
        world_names_.insert(dir_path.filename());
      }
    }
  }
  for (const auto& el : world_names_) {
    spdlog::info("{}", el);
  }
}

bool WorldManager::WorldExists(const std::string& name) { return world_names_.contains(name); }
