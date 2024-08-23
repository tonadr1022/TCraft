#pragma once

constexpr const int kChunkLength = 32;
constexpr const int kLOD1ChunkLength = 16;
constexpr const int kChunkLengthM1 = kChunkLength - 1;
constexpr const int kChunkArea = kChunkLength * kChunkLength;
constexpr const int kChunkVolume = kChunkArea * kChunkLength;
constexpr const int kMeshChunkLength = kChunkLength + 2;
constexpr const int kMeshChunkArea = kMeshChunkLength * kMeshChunkLength;
constexpr const int kMeshChunkVolume = kMeshChunkArea * kMeshChunkLength;
constexpr const int kNumVerticalChunks = 8;
constexpr const int kMaxBlockHeight = kChunkLength * kNumVerticalChunks;
using BlockType = uint16_t;
class ChunkData;
class Chunk;

using BlockTypeArray = std::array<BlockType, kChunkVolume>;
using BlockTypeArrayLOD1 = std::array<BlockType, kChunkVolume / 8>;

using ChunkNeighborArray = std::array<std::shared_ptr<Chunk>, 27>;
using ChunkStackArray = std::array<std::shared_ptr<Chunk>, kNumVerticalChunks>;

enum class ChunkState { kNone, kTerrainQueued, kTerrainFinished, kMeshQueued, kMeshFinished };
enum class LODLevel { kNoMesh, kRegular, kOne, kTwo, kThree };

struct ChunkVertex {
  uint32_t data1;
  uint32_t data2;
};
enum class ChunkMapMode { kChunkState, kLODLevels, kCount };
