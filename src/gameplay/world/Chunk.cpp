#include "Chunk.hpp"

Chunk::Chunk(const glm::ivec3& pos) : pos_(pos) {}

const glm::ivec3& Chunk::GetPos() const { return pos_; }
ChunkData& Chunk::GetData() { return data_; }
const Mesh& Chunk::GetMesh() const { return mesh_; }
BlockType Chunk::GetBlock(const glm::ivec3& pos) const { return data_.GetBlock(pos); }
