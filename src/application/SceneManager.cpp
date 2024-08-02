#include "SceneManager.hpp"

#include "EAssert.hpp"
#include "gameplay/scene/BlockEditorScene.hpp"
#include "gameplay/scene/MainMenuScene.hpp"
#include "gameplay/scene/WorldScene.hpp"
#include "renderer/Renderer.hpp"

SceneManager::SceneManager(Window& window)
    : scene_creators_(
          {{"main_menu", [this]() { return std::make_unique<MainMenuScene>(*this); }},
           {"block_editor", [this]() { return std::make_unique<BlockEditorScene>(*this); }}}),
      window_(window) {}

void SceneManager::LoadScene(const std::string& name) {
  EASSERT_MSG(scene_creators_.count(name), "Scene not found");
  // call destructor of active scene
  active_scene_ = nullptr;
  // construct new scene
  active_scene_ = scene_creators_.at(name)();
  if (next_scene_after_scene_construction_err_ != "") {
    auto tmp = next_scene_after_scene_construction_err_;
    next_scene_after_scene_construction_err_ = "";
    LoadScene(tmp);
  }
}

Scene& SceneManager::GetActiveScene() {
  EASSERT_MSG(active_scene_ != nullptr, "Active scene must exist");
  return *active_scene_;
}

void SceneManager::Shutdown() { active_scene_ = nullptr; }

void SceneManager::LoadWorld(std::string_view path) {
  // call destructor of active scene
  active_scene_ = nullptr;
  // construct new scene
  active_scene_ = std::make_unique<WorldScene>(*this, path);
  if (next_scene_after_scene_construction_err_ != "") {
    auto tmp = next_scene_after_scene_construction_err_;
    next_scene_after_scene_construction_err_ = "";
    LoadScene(tmp);
  }
}

void SceneManager::SetNextSceneOnConstructionError(const std::string& name) {
  next_scene_after_scene_construction_err_ = name;
}
