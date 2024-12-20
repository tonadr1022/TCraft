#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "gameplay/world/ChunkDef.hpp"
class ChunkData;
struct Terrain;

class TerrainGenerator {
 public:
  explicit TerrainGenerator(const std::array<std::shared_ptr<Chunk>, kNumVerticalChunks>& chunks,
                            const glm::ivec2& chunk_world_pos, int seed, const Terrain& terrain);
  void GenerateBiome();
  void GenerateSolid(BlockType block);
  void GenerateYLayer(int layer, BlockType block);

 private:
  const std::array<std::shared_ptr<Chunk>, kNumVerticalChunks>& chunks_;
  const Terrain& terrain_;
  glm::ivec2 chunk_world_pos_;
  int seed_;
  void SetBlock(int x, int y, int z, BlockType block);
};

class SingleChunkTerrainGenerator {
 public:
  explicit SingleChunkTerrainGenerator(ChunkData& chunk, const glm::ivec3& chunk_world_pos,
                                       int seed, const Terrain& terrain);
  void GenerateYLayer(int layer, BlockType block);
  void GenerateSolid(BlockType block);
  void GenerateChecker(BlockType block);
  void GenerateChecker(std::vector<BlockType>& blocks);
  void GenerateLayers(std::vector<BlockType>& blocks);
  void GenerateLayers(BlockType block);
  void GeneratePyramid(std::vector<BlockType>& blocks);
  void GenerateNoise(BlockType block, float frequency);

 private:
  ChunkData& chunk_;
  const Terrain& terrain_;
  glm::ivec3 chunk_world_pos_;
  int seed_;
  void SetBlock(int x, int y, int z, BlockType block);
  void SetBlock(const glm::ivec3& pos, BlockType block);
};
