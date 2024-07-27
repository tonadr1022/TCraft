#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct ChunkVertex {
  glm::vec3 position;
  glm::vec3 tex_coords;
};

class ChunkMesh {
 public:
  uint32_t handle_{UINT32_MAX};
};
