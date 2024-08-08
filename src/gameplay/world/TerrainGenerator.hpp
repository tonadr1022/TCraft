#pragma once

#include <glm/vec3.hpp>

#include "gameplay/world/ChunkDef.hpp"
class ChunkData;

class TerrainGenerator {
 public:
  explicit TerrainGenerator(ChunkData& chunk, const glm::ivec3& chunk_world_pos);
  void GenerateSolid(BlockType block);
  void GenerateChecker(BlockType block);
  void GenerateChecker(std::vector<BlockType>& blocks);
  void GenerateLayers(std::vector<BlockType>& blocks);
  void GenerateLayers(BlockType block);
  void GeneratePyramid(std::vector<BlockType>& blocks);

 private:
  ChunkData& chunk_;
  glm::ivec3 chunk_world_pos_;
  void SetBlock(int x, int y, int z, BlockType block);
  void SetBlock(const glm::ivec3& pos, BlockType block);
};
