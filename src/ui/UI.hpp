#pragma once

#include <glm/vec2.hpp>

#include "Button.hpp"

namespace ui {

class UI {
 public:
  static bool Button();

 private:
  static glm::ivec2 mouse_pos_;
  std::vector<ui::Button> buttons_;
};

}  // namespace ui
