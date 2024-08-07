#pragma once

constexpr const int ChunkLength = 32;
constexpr const int ChunkLengthM1 = ChunkLength - 1;
constexpr const int ChunkArea = ChunkLength * ChunkLength;
constexpr const int ChunkVolume = ChunkArea * ChunkLength;
constexpr const int MeshChunkLength = ChunkLength + 2;
constexpr const int MeshChunkArea = MeshChunkLength * MeshChunkLength;
constexpr const int MeshChunkVolume = MeshChunkArea * MeshChunkLength;
using BlockType = uint16_t;

using BlockTypeArray = BlockType[ChunkVolume];
using ChunkNeighborArray = BlockType (*[27])[ChunkVolume];