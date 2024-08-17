#pragma once

#include <glm/vec3.hpp>

#include "gameplay/world/ChunkDef.hpp"

static inline uint8_t ChunkNeighborOffsetToIdx(int x, int y, int z) {
  return 9 * (y + 1) + 3 * (z + 1) + (x + 1);
}

namespace chunk {

static inline int GetIndex(const glm::ivec3& pos) { return pos.y << 10 | pos.z << 5 | pos.x; }
static inline int GetIndex(int x, int y, int z) { return y << 10 | z << 5 | x; }

static inline int MostCommonNonAirBlock(const std::span<BlockType, 8>& arr) {
  ZoneScoped;
  int most_common = 0;
  for (int i = 0; i < 8; i++) {
    if (arr[i] != 0) {
      most_common = arr[i];
    }
  }
  int max_count = 0;

  for (int i = 0; i < 8; ++i) {
    int count = 0;
    for (int j = 0; j < 8; ++j) {
      if (arr[i] == arr[j] && arr[i] != 0) {
        count++;
      }
    }
    if (count > max_count) {
      max_count = count;
      most_common = arr[i];
    }
  }

  return most_common;
}

}  // namespace chunk
