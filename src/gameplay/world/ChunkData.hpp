#pragma once

#include <array>
#include <glm/fwd.hpp>

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

  // Helpers
  [[nodiscard]] static int GetIndex(const glm::ivec3& pos);
  [[nodiscard]] static int GetIndex(int x, int y, int z);
  static bool IsOutOfBounds(const glm::ivec3& pos);
  static bool IsOutOfBounds(int x, int y, int z);

 private:
  friend class Chunk;
  friend class TerrainGenerator;
  int non_air_block_count_{0};
  ChunkArray blocks_{0};
};
