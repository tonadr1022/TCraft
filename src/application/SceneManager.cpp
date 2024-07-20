#include "SceneManager.hpp"

#include "EAssert.hpp"
#include "gameplay/scene/MainMenuScene.hpp"
#include "gameplay/scene/WorldScene.hpp"

SceneManager::SceneManager()
    : scene_creators_(
          {{"world", [this]() { return std::make_unique<WorldScene>(*this); }},
           {"main_menu", [this]() { return std::make_unique<MainMenuScene>(*this); }}}) {}

void SceneManager::LoadScene(const std::string& name) {
  EASSERT_MSG(scene_creators_.count(name), "Scene not found");
  active_scene_ = scene_creators_.at(name)();
}

Scene& SceneManager::GetActiveScene() {
  EASSERT_MSG(active_scene_ != nullptr, "Active scene must exist");
  return *active_scene_;
}

void SceneManager::Shutdown() { active_scene_ = nullptr; }
