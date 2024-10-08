#pragma once

#include "application/EventDispatcher.hpp"
#include "application/SceneManager.hpp"
#include "application/Window.hpp"
#include "renderer/Renderer.hpp"

class Application {
 public:
  Application(int width, int height, const char* title);
  ~Application();
  void Run();
  void OnEvent(const SDL_Event& event);
  void OnImGui();

 private:
  bool imgui_enabled_;
  Window window_;
  SceneManager scene_manager_;
  EventDispatcher event_dispatcher_;
  void ImGuiTest();
};
