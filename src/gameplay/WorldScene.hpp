#pragma once

#include "application/Scene.hpp"
class WorldScene : public Scene {
 public:
  WorldScene() = default;
  void Init() override;
  void Update(double dt) override;
  void OnImGui() const override;
  bool OnEvent(const SDL_Event& event) override;
};
