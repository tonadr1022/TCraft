#include "Chunk.hpp"

Chunk::Chunk(const glm::ivec3& pos) : pos_(pos) {}

const glm::ivec3& Chunk::GetPos() const { return pos_; }
ChunkData& Chunk::GetData() { return data_; }
ChunkMesh& Chunk::GetMesh() { return mesh_; }
