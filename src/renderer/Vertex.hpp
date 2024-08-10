#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct Vertex {
  glm::vec3 position;
  glm::vec2 tex_coords;
};

struct ColorVertex {
  glm::vec3 position;
  glm::vec3 color;
};
