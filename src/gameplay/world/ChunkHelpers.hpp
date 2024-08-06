#pragma once

#include <glm/vec3.hpp>

static inline uint8_t ChunkNeighborOffsetToIdx(int x, int y, int z) {
  return 9 * (y + 1) + 3 * (z + 1) + (x + 1);
}

namespace chunk {

static inline int GetIndex(const glm::ivec3& pos) { return pos.y << 10 | pos.z << 5 | pos.x; }
static inline int GetIndex(int x, int y, int z) { return y << 10 | z << 5 | x; }

}  // namespace chunk
