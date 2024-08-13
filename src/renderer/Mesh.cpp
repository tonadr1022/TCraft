#include "Mesh.hpp"

#include "renderer/Renderer.hpp"
#include "renderer/Vertex.hpp"

Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
  Allocate(vertices, indices);
}

void Mesh::Allocate(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
  EASSERT_MSG(!IsAllocated(), "Cannot be allocated already");
  handle_ = Renderer::Get().AllocateMesh(vertices, indices);
  vertex_count_ = vertices.size();
  index_count_ = indices.size();
}

Mesh::~Mesh() {
  if (handle_) {
    Renderer::Get().FreeRegMesh(handle_);
  }
  handle_ = 0;
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

void Mesh::Free() {
  // TODO: maybe allow double free?
  EASSERT_MSG(handle_, "Don't explicitly free a non allocated mesh");
  if (handle_) {
    Renderer::Get().FreeRegMesh(handle_);
  }
  handle_ = 0;
}
