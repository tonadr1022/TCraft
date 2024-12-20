#include "SceneManager.hpp"

#include "EAssert.hpp"
#include "gameplay/scene/BlockEditorScene.hpp"
#include "gameplay/scene/MainMenuScene.hpp"
#include "gameplay/scene/ShadowScene.hpp"
#include "gameplay/scene/WorldScene.hpp"
#include "renderer/Renderer.hpp"
#include "resource/MaterialManager.hpp"
#include "resource/TextureManager.hpp"

SceneManager::SceneManager(Window& window)
    : scene_creators_({
          {"main_menu", [this]() { return std::make_unique<MainMenuScene>(*this); }},
          {"block_editor", [this]() { return std::make_unique<BlockEditorScene>(*this); }},
          {"shadow", [this]() { return std::make_unique<ShadowScene>(*this); }},
      }),
      window_(window) {}

void SceneManager::LoadScene(const std::string& name) {
  EASSERT_MSG(scene_creators_.count(name), "Scene not found");
  // call destructor of active scene
  active_scene_ = nullptr;
  Renderer::Get().SetSkyboxShaderFunc({});
  // construct new scene
  active_scene_ = scene_creators_.at(name)();

  // Up to the user to end this loop with a valid constructed scene name if the
  // scene they intend fails to load
  while (next_scene_after_scene_construction_err_ != "") {
    // call destructor of active scene
    active_scene_ = nullptr;
    next_scene_after_scene_construction_err_ = "";
    // construct new scene
    active_scene_ = scene_creators_.at(name)();
  }

  MaterialManager::Get().RemoveUnused();
  TextureManager::Get().RemoveUnusedTextures();
  // TODO: make a reset function instead?
}

Scene& SceneManager::GetActiveScene() {
  EASSERT_MSG(active_scene_ != nullptr, "Active scene must exist");
  return *active_scene_;
}

void SceneManager::Shutdown() {
  ZoneScoped;
  active_scene_ = nullptr;
  MaterialManager::Get().RemoveUnused();
  TextureManager::Get().RemoveUnusedTextures();
}

void SceneManager::LoadWorld(const std::string& path) {
  // call destructor of active scene
  active_scene_ = nullptr;
  Renderer::Get().SetSkyboxShaderFunc({});
  // construct new scene
  active_scene_ = std::make_unique<WorldScene>(*this, path);
  if (next_scene_after_scene_construction_err_ != "") {
    auto tmp = next_scene_after_scene_construction_err_;
    next_scene_after_scene_construction_err_ = "";
    LoadScene(tmp);
  } else {
    MaterialManager::Get().RemoveUnused();
    TextureManager::Get().RemoveUnusedTextures();
    // TODO: make a reset function instead?
  }
}

void SceneManager::SetNextSceneOnConstructionError(const std::string& name) {
  next_scene_after_scene_construction_err_ = name;
}
