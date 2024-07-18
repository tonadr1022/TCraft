#include "SceneManager.hpp"

#include "EAssert.hpp"
#include "gameplay/scene/MainMenuScene.hpp"
#include "gameplay/scene/WorldScene.hpp"

void SceneManager::LoadScene(const std::string& name) {
  if (name == "world") {
    active_scene_ = std::make_unique<WorldScene>(*this);
  } else if (name == "main_menu") {
    active_scene_ = std::make_unique<MainMenuScene>(*this);
  } else {
    EASSERT_MSG(false, "Scene does not exist");
  }
}

Scene& SceneManager::GetActiveScene() {
  EASSERT_MSG(active_scene_ != nullptr, "Active scene must exist");
  return *active_scene_;
}

void SceneManager::Shutdown() { active_scene_ = nullptr; }
