#pragma once

#include "application/Scene.hpp"

class SceneManager {
 public:
  // Loads a scene from the map by name, other than worlds
  void LoadScene(const std::string& name);
  Scene& GetActiveScene();
  void Shutdown();
  void LoadWorld(const std::string& world_name);

 private:
  // Initialization
  friend class Application;
  // Allows scenes to initialize window ref
  friend class Scene;
  explicit SceneManager(Window& window);

  std::unordered_map<std::string, std::function<std::unique_ptr<Scene>()>> scene_creators_;
  std::unique_ptr<Scene> active_scene_{nullptr};
  Window& window_;
};
