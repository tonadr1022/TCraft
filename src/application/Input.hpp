#pragma once

#include <SDL_keycode.h>
#include <SDL_mouse.h>

#include <map>

class Input {
 public:
  static inline bool IsKeyDown(SDL_Keycode key) { return key_states_[key]; }
  static bool IsMouseButtonPressed(int button) { return mouse_button_states_[button]; }

 private:
  friend class Application;
  static inline void SetKeyPressed(SDL_Keycode key, bool pressed) { key_states_[key] = pressed; };
  static inline void SetMouseButtonPressed(uint32_t button, bool pressed) {
    mouse_button_states_[button] = pressed;
  }
  static inline std::map<SDL_Keycode, bool> key_states_;
  static inline std::map<uint32_t, bool> mouse_button_states_;
};
