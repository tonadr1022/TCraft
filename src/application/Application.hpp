#pragma once

#include "application/EventDispatcher.hpp"
#include "application/Window.hpp"
#include "gameplay/world/World.hpp"
#include "renderer/Renderer.hpp"

class Settings;

class Application {
 public:
  Application(int width, int height, const char* title);
  ~Application();
  void Run();
  void OnEvent(const SDL_Event& event);
  void OnImGui();

 private:
  Settings* settings_{nullptr};
  Renderer renderer_;
  Window window_;
  World world_;
  EventDispatcher event_dispatcher_;
  void ImGuiTest();
};
