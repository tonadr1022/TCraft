#pragma once

struct Vertex {
  glm::vec3 position;
  glm::vec2 tex_coords;
};

struct ColorVertex {
  glm::vec3 position;
  glm::vec3 color;
};
