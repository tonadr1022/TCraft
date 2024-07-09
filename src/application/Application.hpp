#pragma once

#include "application/Window.hpp"

class Application {
 public:
  Application(int width, int height, const char* title);
  void Run();
  void OnEvent(SDL_Event& event);

 private:
  Window window_;
};
