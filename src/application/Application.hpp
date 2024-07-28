#pragma once

#include "application/EventDispatcher.hpp"
#include "application/SceneManager.hpp"
#include "application/Window.hpp"
#include "renderer/Renderer.hpp"

class SettingsManager;
class TextureManager;

class Application {
 public:
  Application(int width, int height, const char* title);
  ~Application();
  void Run();
  void OnEvent(const SDL_Event& event);
  void OnImGui();

 private:
  bool imgui_enabled_;
  SettingsManager* settings_{nullptr};
  TextureManager* texture_manager_{nullptr};
  Renderer* renderer_{nullptr};
  SceneManager scene_manager_;
  Window window_;
  EventDispatcher event_dispatcher_;
  void ImGuiTest();
};
