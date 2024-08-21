#pragma once

#include <glm/mat4x4.hpp>

struct RenderInfo {
  glm::mat4 vp_matrix;
  glm::mat4 view_matrix;
  glm::mat4 proj_matrix;
  glm::vec3 view_pos;
  glm::vec3 view_dir;
};
