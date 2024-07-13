#pragma once

#include <SDL_events.h>

#include <cstdint>
#include <glm/fwd.hpp>

struct SDL_Window;

using EventCallback = std::function<void(SDL_Event&)>;

class Window {
 public:
  void Init(int width, int height, const char* title, EventCallback event_callback);
  void SetVsync(bool vsync);
  void Shutdown();
  void SetCursorVisible(bool cursor_visible);
  void SetUserPointer(void* ptr);
  void StartFrame();
  void EndFrame() const;

  static std::string GetEventTypeString(Uint32 eventType);

  bool imgui_enabled{true};
  [[nodiscard]] bool GetVsync() const;
  [[nodiscard]] bool ShouldClose() const;

 private:
  EventCallback event_callback_;
  SDL_Window* window_{nullptr};
  bool should_close_{false};
  uint32_t framebuffer_width_;
  uint32_t framebuffer_height_;
  bool vsync_on_{true};
};
