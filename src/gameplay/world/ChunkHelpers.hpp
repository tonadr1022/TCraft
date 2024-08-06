#pragma once

#include "ChunkData.hpp"

static inline int MeshIndex(int x, int y, int z) {
  return (x + 1) + ((y + 1 + (z + 1 * MeshChunkLength)) * MeshChunkLength);
}

static inline uint8_t ChunkNeighborOffsetToIdx(int x, int y, int z) {
  return 9 * (y + 1) + 3 * (z + 1) + (x + 1);
}

namespace chunk {

static inline int GetIndex(glm::ivec3 pos) { return pos.y << 10 | pos.z << 5 | pos.x; }
static inline int GetIndex(int x, int y, int z) { return y << 10 | z << 5 | x; }

}  // namespace chunk
