#pragma once

#include "application/EventDispatcher.hpp"
#include "application/Window.hpp"
#include "gameplay/world/World.hpp"

class ShaderManager;
class Settings;
class Renderer;
class InternalSettings;
class BlockDB;

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
  InternalSettings* internal_settings_{nullptr};
  Renderer* renderer_{nullptr};
  BlockDB* block_db_{nullptr};
  Window window_;
  World world_;
  EventDispatcher event_dispatcher_;
  void ImGuiTest();
};
