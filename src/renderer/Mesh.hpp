#pragma once

#include <glm/vec3.hpp>

struct ChunkVertex {
  glm::vec3 position;
  glm::vec3 tex_coords;
};
struct Vertex;

class Mesh {
 public:
  ~Mesh();
  Mesh();
  Mesh(std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices);
  Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
  void Allocate(std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices);
  void Allocate(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

  Mesh(Mesh& other) = delete;
  Mesh& operator=(Mesh& other) = delete;
  Mesh(Mesh&& other) noexcept;
  Mesh& operator=(Mesh&& other) noexcept;

  [[nodiscard]] uint32_t Handle() const;
  [[nodiscard]] inline bool IsAllocated() const { return handle_ != 0; };

 private:
  uint32_t handle_{0};
};
