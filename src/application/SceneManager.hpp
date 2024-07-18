#pragma once

#include "application/Scene.hpp"

class SceneManager {
 public:
  void LoadScene(const std::string& name);
  Scene& GetActiveScene();
  void Shutdown();

 private:
  friend class Application;
  SceneManager() = default;

  std::unique_ptr<Scene> active_scene_{nullptr};
};
