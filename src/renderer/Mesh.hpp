#pragma once

#include <glm/vec3.hpp>

struct Vertex;

class Mesh {
 public:
  Mesh();
  ~Mesh();
  Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
  void Allocate(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
  void Free();

  Mesh(Mesh& other) = delete;
  Mesh& operator=(Mesh& other) = delete;
  Mesh(Mesh&& other) noexcept;
  Mesh& operator=(Mesh&& other) noexcept;

  [[nodiscard]] uint32_t Handle() const;
  [[nodiscard]] inline bool IsAllocated() const { return handle_ != 0; };

  uint32_t handle_{0};
  uint32_t vertex_count_{0};
  uint32_t index_count_{0};

 private:
};
