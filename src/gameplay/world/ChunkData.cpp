#include "ChunkData.hpp"

#include <glm/vec3.hpp>
#include <memory>

#include "gameplay/world/ChunkDef.hpp"
#include "gameplay/world/ChunkHelpers.hpp"

ChunkData::ChunkData(const ChunkData& other)
    : block_count_(other.block_count_),
      lod_needs_refresh_(other.lod_needs_refresh_),
      has_lod_1_(other.has_lod_1_) {
  if (other.blocks_) {
    blocks_ = std::make_unique<BlockTypeArray>(*other.blocks_);
  }
  if (other.blocks_lod_1_) {
    blocks_lod_1_ = std::make_unique<BlockTypeArrayLOD1>(*other.blocks_lod_1_);
  }
}

ChunkData::ChunkData(ChunkData&& other) noexcept
    : blocks_(std::move(other.blocks_)),
      blocks_lod_1_(std::move(other.blocks_lod_1_)),
      block_count_(other.block_count_),
      lod_needs_refresh_(other.lod_needs_refresh_),
      has_lod_1_(other.has_lod_1_) {}

ChunkData& ChunkData::operator=(const ChunkData& other) {
  block_count_ = other.block_count_;
  lod_needs_refresh_ = other.lod_needs_refresh_;
  has_lod_1_ = other.has_lod_1_;
  if (other.blocks_) {
    blocks_ = std::make_unique<BlockTypeArray>(*other.blocks_);
  }
  if (other.blocks_lod_1_) {
    blocks_lod_1_ = std::make_unique<BlockTypeArrayLOD1>(*other.blocks_lod_1_);
  }
  return *this;
}

ChunkData& ChunkData::operator=(ChunkData&& other) noexcept {
  block_count_ = other.block_count_;
  lod_needs_refresh_ = other.lod_needs_refresh_;
  has_lod_1_ = other.has_lod_1_;
  blocks_ = std::move(other.blocks_);
  blocks_lod_1_ = std::move(other.blocks_lod_1_);
  return *this;
}

BlockType ChunkData::GetBlockLOD1(int x, int y, int z) const {
  if (blocks_lod_1_ == nullptr) return 0;
  return (*blocks_lod_1_)[x + z * 16 + y * 256];
}

BlockType ChunkData::GetBlock(int x, int y, int z) const {
  if (blocks_ == nullptr) return 0;
  return (*blocks_)[GetIndex(x, y, z)];
}

BlockType ChunkData::GetBlock(const glm::ivec3& pos) const {
  if (blocks_ == nullptr) return 0;
  return (*blocks_)[GetIndex(pos)];
}

void ChunkData::SetBlock(int x, int y, int z, BlockType block) {
  if (blocks_ == nullptr) blocks_ = std::make_unique<BlockTypeArray>();
  int index = GetIndex(x, y, z);
  BlockType curr = (*blocks_)[index];
  block_count_ += (curr == 0 && block != 0);
  block_count_ -= (curr != 0 && block == 0);
  lod_needs_refresh_ = true;
  (*blocks_)[index] = block;
}

void ChunkData::SetBlock(const glm::ivec3& pos, BlockType block) {
  return SetBlock(pos.x, pos.y, pos.z, block);
}

int ChunkData::GetBlockCount() const { return block_count_; }

void ChunkData::DownSample() {
  ZoneScoped;
  if (has_lod_1_ && !lod_needs_refresh_) return;
  has_lod_1_ = true;
  lod_needs_refresh_ = false;
  if (block_count_ == 0) return;
  if (blocks_lod_1_ == nullptr) blocks_lod_1_ = std::make_unique<BlockTypeArrayLOD1>();
  uint32_t f = 2;
  // TODO: templatize downsampling
  uint32_t chunk_length = kChunkLength / f;
  auto get_index = [](int x, int y, int z) -> int { return x | z << 4 | y << 8; };

  std::array<BlockType, 8> types;
  for (uint32_t new_y = 0; new_y < chunk_length; new_y++) {
    for (uint32_t new_z = 0; new_z < chunk_length; new_z++) {
      for (uint32_t new_x = 0; new_x < chunk_length; new_x++) {
        // accumulate
        int i = 0;
        for (uint32_t dy = 0; dy < f; dy++) {
          for (uint32_t dz = 0; dz < f; dz++) {
            for (uint32_t dx = 0; dx < f; dx++, i++) {
              types[i] =
                  (*blocks_)[chunk::GetIndex(new_x * f + dx, new_y * f + dy, new_z * f + dz)];
            }
          }
        }

        (*blocks_lod_1_)[get_index(new_x, new_y, new_z)] = chunk::MostCommonNonAirBlock(types);
        types.fill(0);
      }
    }
  }
}
