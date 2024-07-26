#pragma once

#include "gameplay/world/Block.hpp"
class ChunkData;

class TerrainGenerator {
 public:
  static void GenerateSolid(ChunkData& chunk, BlockType block);
  static void GenerateChecker(ChunkData& chunk, BlockType block);
  static void GenerateChecker(ChunkData& chunk, std::vector<BlockType>& blocks);
  static void GeneratePyramid(ChunkData& chunk, std::vector<BlockType>& blocks);
};
