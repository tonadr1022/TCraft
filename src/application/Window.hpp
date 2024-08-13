#pragma once

#include <SDL_events.h>

#include <cstdint>
#include <glm/fwd.hpp>

struct SDL_Window;

using EventCallback = std::function<void(SDL_Event&)>;

class Window {
 public:
  void Init(int width, int height, const char* title, const EventCallback& event_callback);
  void SetVsync(bool vsync);
  void Shutdown();
  void SetCursorVisible(bool cursor_visible);
  void CenterCursor();
  void SetUserPointer(void* ptr);
  void StartRenderFrame(bool imgui_enabled);
  void EndRenderFrame(bool imgui_enabled) const;
  void PollEvents();
  void SetMouseGrab(bool state);
  void SetTitle(std::string_view title);
  static void DisableImGuiInputs();
  static void EnableImGuiInputs();
  static std::string GetEventTypeString(Uint32 eventType);

  [[nodiscard]] float GetAspectRatio() const;
  [[nodiscard]] bool GetVsync() const;
  [[nodiscard]] bool ShouldClose() const;
  [[nodiscard]] glm::ivec2 GetWindowSize() const;
  [[nodiscard]] glm::ivec2 GetMousePosition() const;
  [[nodiscard]] glm::ivec2 GetWindowCenter() const;

  static Window& Get();

 private:
  friend class Application;
  Window();
  static Window* instance_;

  EventCallback event_callback_;
  SDL_Window* window_{nullptr};
  bool should_close_{false};
  uint32_t framebuffer_width_;
  uint32_t framebuffer_height_;
  bool vsync_on_{true};
};
