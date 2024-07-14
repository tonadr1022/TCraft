#include "ChunkMesh.hpp"

void ChunkMesh::Allocate() {
  vao.EnableAttribute<float>(0, 3, offsetof(ChunkVertex, position));
  vao.EnableAttribute<float>(1, 2, offsetof(ChunkVertex, tex_coords));
  vbo.Init(vertices.size() * sizeof(ChunkVertex), 0, vertices.data());
  ebo.Init(indices.size() * sizeof(uint32_t), 0, indices.data());
  vao.AttachVertexBuffer(vbo.Id(), 0, 0, sizeof(ChunkVertex));
  vao.AttachElementBuffer(ebo.Id());

  // TODO(tony): memory allocator for vertices and indices.

  num_vertices_ = vertices.size();
  vertices.clear();
  vertices.shrink_to_fit();
  num_indices_ = indices.size();
  indices.clear();
  indices.shrink_to_fit();
}
