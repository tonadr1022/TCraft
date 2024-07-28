#include "ChunkMesh.hpp"

#include "renderer/Renderer.hpp"

void ChunkMesh::Allocate(std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices) {
  EASSERT_MSG(!IsAllocated(), "Cannot be allocated already");
  handle_ = Renderer::Get().AllocateChunk(vertices, indices);
}

ChunkMesh::~ChunkMesh() {
  if (handle_) {
    Renderer::Get().FreeChunk(handle_);
  }
}
ChunkMesh::ChunkMesh() = default;

ChunkMesh::ChunkMesh(ChunkMesh&& other) noexcept { *this = std::move(other); }

ChunkMesh& ChunkMesh::operator=(ChunkMesh&& other) noexcept {
  if (&other == this) return *this;
  this->~ChunkMesh();
  handle_ = std::exchange(other.handle_, 0);
  return *this;
}
uint32_t ChunkMesh::Handle() const {
  EASSERT_MSG(handle_, "Don't get a null handle");
  return handle_;
}
