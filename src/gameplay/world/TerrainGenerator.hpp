#pragma once

#include "gameplay/world/Block.hpp"
class ChunkData;

class TerrainGenerator {
 public:
  explicit TerrainGenerator(ChunkData& chunk);
  void GenerateSolid(BlockType block);
  void GenerateChecker(BlockType block);
  void GenerateChecker(std::vector<BlockType>& blocks);
  void GenerateLayers(std::vector<BlockType>& blocks);
  void GeneratePyramid(std::vector<BlockType>& blocks);

  ChunkData& chunk;
};
