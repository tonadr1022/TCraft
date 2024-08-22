#pragma once

#include <glm/vec2.hpp>

#include "Button.hpp"
#include "resource/Resource.hpp"

class UI {
 public:
  static void Begin();
  static void End();
  static bool Button(const glm::vec2& pos, const glm::vec2& size, AssetHandle material_handle);

 private:
  static bool IntersectRect(const glm::vec2& bot_left, const glm::vec2& top_right);
  static glm::ivec2 mouse_pos_;
  inline static bool mouse_clicked_{false};
  inline static bool prev_mouse_clicked_{false};
  std::vector<ui::Button> buttons_;
};
