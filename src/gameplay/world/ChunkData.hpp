#pragma once

#include <glm/vec3.hpp>
#include <memory>

#include "gameplay/world/ChunkDef.hpp"

class ChunkData {
 public:
  ChunkData() = default;
  ~ChunkData() = default;
  ChunkData(const ChunkData& other);
  ChunkData(ChunkData&& other) noexcept;
  ChunkData& operator=(ChunkData&& other) noexcept;
  ChunkData& operator=(const ChunkData& other);

  void SetBlock(const glm::ivec3& pos, BlockType block);
  void SetBlock(int x, int y, int z, BlockType block);
  void SetBlockNoCheck(const glm::ivec3& pos, BlockType block);
  [[nodiscard]] BlockType GetBlockNoCheck(const glm::ivec3& pos) const;
  [[nodiscard]] BlockType GetBlock(const glm::ivec3& pos) const;
  [[nodiscard]] BlockType GetBlock(int x, int y, int z) const;
  [[nodiscard]] BlockType GetBlockLOD1(int x, int y, int z) const;
  [[nodiscard]] BlockTypeArray* GetBlocks() const { return blocks_.get(); }
  [[nodiscard]] inline bool HasLOD1() const { return has_lod_1_; }

  [[nodiscard]] static inline int GetIndex(glm::ivec3 pos) {
    return pos.y * kChunkArea + pos.z * kChunkLength + pos.x;
  }

  [[nodiscard]] static inline int GetIndex(int x, int y, int z) {
    return y * kChunkArea + z * kChunkLength + x;
  }

  // return true if x, y, or z are not between 0-31 inclusive.
  [[nodiscard]] static inline bool IsOutOfBounds(int x, int y, int z) {
    return (x & 0b1111100000) || (y & 0b1111100000) || (z & 0b1111100000);
  }
  // return true if x, y, or z are not between 0-31 inclusive.
  [[nodiscard]] static inline bool IsOutOfBounds(const glm::ivec3& pos) {
    return (pos.x & 0b1111100000) || (pos.y & 0b1111100000) || (pos.z & 0b1111100000);
  }

  std::unique_ptr<BlockTypeArray> blocks_{nullptr};
  std::unique_ptr<BlockTypeArrayLOD1> blocks_lod_1_{nullptr};
  void DownSample();
  [[nodiscard]] int GetBlockCount() const;

 private:
  friend class TerrainGenerator;
  friend class SingleChunkTerrainGenerator;
  int block_count_{0};
  bool lod_needs_refresh_{true};
  bool has_lod_1_{false};
};
