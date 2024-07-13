#pragma once

#include "application/EventDispatcher.hpp"
#include "application/Window.hpp"

class ShaderManager;
class Settings;
class Renderer;

class Application {
 public:
  Application(int width, int height, const char* title);
  ~Application();
  void Run();
  void OnEvent(SDL_Event& event);

 private:
  ShaderManager* shader_manager_{nullptr};
  Settings* settings_{nullptr};
  Renderer* renderer_{nullptr};
  Window window_;
  EventDispatcher event_dispatcher_;
  void LoadShaders();
  void ImGuiTest();
};
