#include "ChunkMesh.hpp"

void ChunkMesh::Allocate() {
  ZoneScoped;
  vao.EnableAttribute<float>(0, 3, offsetof(ChunkVertex, position));
  vao.EnableAttribute<float>(1, 2, offsetof(ChunkVertex, tex_coords));
  vao.EnableAttribute<int>(2, 1, offsetof(ChunkVertex, index));
  vbo.Init(vertices.size() * sizeof(ChunkVertex), 0, vertices.data());
  ebo.Init(indices.size() * sizeof(uint32_t), 0, indices.data());
  vao.AttachVertexBuffer(vbo.Id(), 0, 0, sizeof(ChunkVertex));
  vao.AttachElementBuffer(ebo.Id());

  // TODO: memory allocator for vertices and indices.
  num_vertices_ = vertices.size();
  vertices.clear();
  vertices.shrink_to_fit();
  num_indices_ = indices.size();
  indices.clear();
  indices.shrink_to_fit();

  gpu_allocated = true;
}
