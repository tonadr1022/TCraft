#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

#include "renderer/opengl/Buffer.hpp"
#include "renderer/opengl/VertexArray.hpp"

struct ChunkVertex {
  glm::vec3 position;
  glm::vec2 tex_coords;
};

class ChunkMesh {
 public:
  std::vector<ChunkVertex> vertices;
  std::vector<uint32_t> indices;
  void Allocate();
  bool gpu_allocated{false};
  VertexArray vao;
  Buffer vbo;
  Buffer ebo;
  uint32_t num_vertices_;
  uint32_t num_indices_;
};
