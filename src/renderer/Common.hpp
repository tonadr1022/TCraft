#pragma once

#include <glm/mat4x4.hpp>

struct UserDrawCommand {
  glm::mat4 model;
  uint32_t material_handle;
};
