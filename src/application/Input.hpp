#pragma once

#include <SDL_keycode.h>

#include <map>

class Input {
 public:
  static inline bool IsKeyDown(SDL_Keycode key) { return key_states_[key]; }
  static bool IsMouseButtonPressed(int mouse_button);

 private:
  friend class Application;
  static inline void SetKeyPressed(SDL_Keycode key, bool pressed) { key_states_[key] = pressed; };
  static inline std::map<SDL_Keycode, bool> key_states_;
};
