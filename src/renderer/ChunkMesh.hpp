#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct ChunkVertex {
  glm::vec3 position;
  glm::vec3 tex_coords;
};

class ChunkMesh {
 public:
  ~ChunkMesh();
  ChunkMesh();
  void Allocate(std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices);

  ChunkMesh(ChunkMesh& other) = delete;
  ChunkMesh& operator=(ChunkMesh& other) = delete;
  ChunkMesh(ChunkMesh&& other) noexcept;
  ChunkMesh& operator=(ChunkMesh&& other) noexcept;

  [[nodiscard]] uint32_t Handle() const;
  [[nodiscard]] inline bool IsAllocated() const { return handle_ != 0; };

 private:
  uint32_t handle_{0};
};
