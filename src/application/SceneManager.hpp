#pragma once

#include "application/Scene.hpp"

class SceneManager {
 public:
  // Loads a scene from the map by name, other than worlds
  void LoadScene(const std::string& name);
  Scene& GetActiveScene();
  void Shutdown();
  void LoadWorld(std::string_view path);
  void SetNextSceneOnConstructionError(const std::string& name);

 private:
  // Initialization
  friend class Application;
  // Allows scenes to initialize window ref
  friend class Scene;
  explicit SceneManager(Window& window);

  std::unordered_map<std::string, std::function<std::unique_ptr<Scene>()>> scene_creators_;
  std::unique_ptr<Scene> active_scene_{nullptr};
  Window& window_;
  std::string next_scene_after_scene_construction_err_;
};
