#include "Mesh.hpp"

#include "renderer/Renderer.hpp"

Mesh::Mesh(std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices) {
  Allocate(vertices, indices);
}

Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
  Allocate(vertices, indices);
}

void Mesh::Allocate(std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices) {
  EASSERT_MSG(!IsAllocated(), "Cannot be allocated already");
  handle_ = Renderer::Get().AllocateMesh(vertices, indices);
  type_ = Type::Chunk;
}

void Mesh::Allocate(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
  EASSERT_MSG(!IsAllocated(), "Cannot be allocated already");
  handle_ = Renderer::Get().AllocateMesh(vertices, indices);
  type_ = Type::Regular;
}

Mesh::~Mesh() {
  if (handle_) {
    if (type_ == Type::Regular) {
      Renderer::Get().FreeRegMesh(handle_);
    } else {
      Renderer::Get().FreeChunkMesh(handle_);
    }
  }
  handle_ = 0;
}
Mesh::Mesh() = default;

Mesh::Mesh(Mesh&& other) noexcept { *this = std::move(other); }

Mesh& Mesh::operator=(Mesh&& other) noexcept {
  if (&other == this) return *this;
  this->~Mesh();
  handle_ = std::exchange(other.handle_, 0);
  type_ = other.type_;
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
    if (type_ == Type::Regular) {
      Renderer::Get().FreeRegMesh(handle_);
    } else {
      Renderer::Get().FreeChunkMesh(handle_);
    }
  }
  handle_ = 0;
}
