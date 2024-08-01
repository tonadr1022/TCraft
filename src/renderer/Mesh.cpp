#include "Mesh.hpp"

#include "renderer/Renderer.hpp"

void Mesh::Allocate(std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices) {
  EASSERT_MSG(!IsAllocated(), "Cannot be allocated already");
  handle_ = Renderer::Get().AllocateMesh(vertices, indices);
}

Mesh::Mesh(std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices) {
  handle_ = Renderer::Get().AllocateMesh(vertices, indices);
}

void Mesh::Allocate(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
  EASSERT_MSG(!IsAllocated(), "Cannot be allocated already");
  handle_ = Renderer::Get().AllocateMesh(vertices, indices);
}

Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
  handle_ = Renderer::Get().AllocateMesh(vertices, indices);
}

Mesh::~Mesh() {
  if (handle_) {
    Renderer::Get().FreeChunkMesh(handle_);
  }
}
Mesh::Mesh() = default;

Mesh::Mesh(Mesh&& other) noexcept { *this = std::move(other); }

Mesh& Mesh::operator=(Mesh&& other) noexcept {
  if (&other == this) return *this;
  this->~Mesh();
  handle_ = std::exchange(other.handle_, 0);
  return *this;
}
uint32_t Mesh::Handle() const {
  EASSERT_MSG(handle_, "Don't get a null handle");
  return handle_;
}
