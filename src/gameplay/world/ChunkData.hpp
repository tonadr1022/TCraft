#pragma once

#include <array>
#include <glm/vec3.hpp>

#include "Block.hpp"

constexpr const int ChunkLength = 32;
constexpr const int ChunkArea = ChunkLength * ChunkLength;
constexpr const int ChunkVolume = ChunkArea * ChunkLength;

using ChunkArray = std::array<BlockType, ChunkVolume>;

class ChunkData {
 public:
  void SetBlock(const glm::ivec3& pos, BlockType block);
  [[nodiscard]] BlockType GetBlock(const glm::ivec3& pos) const;
  [[nodiscard]] const ChunkArray& GetBlocks() const { return blocks_; }

  [[nodiscard]] static inline int GetIndex(glm::ivec3 pos) {
    return pos.y << 10 | pos.z << 5 | pos.x;
  }
  [[nodiscard]] static inline int GetIndex(int x, int y, int z) { return y << 10 | z << 5 | x; }
  // return true if x, y, or z are not between 0-31 inclusive.
  [[nodiscard]] static inline bool IsOutOfBounds(int x, int y, int z) {
    return (x & 0b1111100000) || (y & 0b1111100000) || (z & 0b1111100000);
  }
  // return true if x, y, or z are not between 0-31 inclusive.
  [[nodiscard]] static inline bool IsOutOfBounds(const glm::ivec3& pos) {
    return (pos.x & 0b1111100000) || (pos.y & 0b1111100000) || (pos.z & 0b1111100000);
  }

 private:
  friend class TerrainGenerator;
  int non_air_block_count_{0};
  ChunkArray blocks_{0};
};
