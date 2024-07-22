#pragma once

class ChunkData;
enum class BlockType;

class TerrainGenerator {
 public:
  static void GenerateSolid(ChunkData& chunk, BlockType block);
  static void GenerateChecker(ChunkData& chunk, BlockType block);
  static void GenerateChecker(ChunkData& chunk, std::vector<BlockType>& blocks);
};
