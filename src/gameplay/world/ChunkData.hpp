#pragma once

#include <glm/vec3.hpp>
#include <memory>

#include "gameplay/world/ChunkDef.hpp"

class ChunkData {
 public:
  void SetBlock(const glm::ivec3& pos, BlockType block);
  void SetBlock(int x, int y, int z, BlockType block);
  void SetBlockNoCheck(const glm::ivec3& pos, BlockType block);
  [[nodiscard]] BlockType GetBlockNoCheck(const glm::ivec3& pos) const;
  [[nodiscard]] BlockType GetBlock(const glm::ivec3& pos) const;
  [[nodiscard]] BlockType GetBlock(int x, int y, int z) const;
  [[nodiscard]] BlockTypeArray* GetBlocks() const { return blocks_.get(); }

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

  // TODO: remove getter if this is going to remain public
  // BlockTypeArray blocks_{0};
  std::unique_ptr<std::array<BlockType, ChunkVolume>> blocks_{nullptr};
  [[nodiscard]] int GetBlockCount() const;

 private:
  friend class TerrainGenerator;
  friend class SingleChunkTerrainGenerator;
  int block_count_{0};
};
