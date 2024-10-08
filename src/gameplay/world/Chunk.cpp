#include "Chunk.hpp"

Chunk::Chunk(const glm::ivec3& pos) : pos_(pos) {}

const glm::ivec3& Chunk::GetPos() const { return pos_; }
