#include "UI.hpp"

#include "application/Window.hpp"
#include "renderer/Renderer.hpp"

bool UI::Button(const glm::vec2& pos, const glm::vec2& size, AssetHandle material_handle) {
  Renderer::Get().DrawQuad(material_handle, pos, size);
  if (IntersectRect(pos - size, pos + size)) {
    return mouse_clicked_;
  }
  return false;
}

bool UI::IntersectRect(const glm::vec2& bot_left, const glm::vec2& top_right) {
  return mouse_pos_.x >= bot_left.x && mouse_pos_.y >= bot_left.y && mouse_pos_.y <= top_right.y &&
         mouse_pos_.x <= top_right.x;
}

void UI::Begin() {
  mouse_pos_ = Window::Get().GetMousePosition();
  mouse_clicked_ = !prev_mouse_clicked_ && SDL_BUTTON(SDL_BUTTON_LEFT);
}

void UI::End() {
  mouse_pos_ = glm::ivec2{-1, -1};
  prev_mouse_clicked_ = mouse_clicked_;
  mouse_clicked_ = false;
}
