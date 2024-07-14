#pragma once

#include "application/EventDispatcher.hpp"
#include "application/Window.hpp"
#include "gameplay/Player.hpp"

class ShaderManager;
class Settings;
class Renderer;

class Application {
 public:
  Application(int width, int height, const char* title);
  ~Application();
  void Run();
  void OnEvent(const SDL_Event& event);
  void OnImGui();

 private:
  ShaderManager* shader_manager_{nullptr};
  Settings* settings_{nullptr};
  Renderer* renderer_{nullptr};
  Window window_;
  Player player_;
  EventDispatcher event_dispatcher_;
  void ImGuiTest();
};
