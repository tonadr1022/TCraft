#pragma once

constexpr const int ChunkLength = 32;
constexpr const int LOD1ChunkLength = 16;
constexpr const int ChunkLengthM1 = ChunkLength - 1;
constexpr const int ChunkArea = ChunkLength * ChunkLength;
constexpr const int ChunkVolume = ChunkArea * ChunkLength;
constexpr const int MeshChunkLength = ChunkLength + 2;
constexpr const int MeshChunkArea = MeshChunkLength * MeshChunkLength;
constexpr const int MeshChunkVolume = MeshChunkArea * MeshChunkLength;
constexpr const int NumVerticalChunks = 8;
constexpr const int MaxBlockHeight = ChunkLength * NumVerticalChunks;
using BlockType = uint16_t;
class ChunkData;
class Chunk;

using BlockTypeArray = std::array<BlockType, ChunkVolume>;
using BlockTypeArrayLOD1 = std::array<BlockType, ChunkVolume / 8>;

using ChunkNeighborArray = std::array<std::shared_ptr<Chunk>, 27>;
using ChunkStackArray = std::array<std::shared_ptr<Chunk>, NumVerticalChunks>;

enum class ChunkState { None, TerrainQueued, TerrainFinished, MeshQueued, MeshFinished };
enum class LODLevel { NoMesh, Regular, One, Two, Three };

struct ChunkVertex {
  uint32_t data1;
  uint32_t data2;
};
