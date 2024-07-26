#pragma once

#include <glm/fwd.hpp>

#include "gameplay/world/Block.hpp"

struct ChunkVertex;
class ChunkData;
class BlockDB;

class ChunkMesher {
 public:
  void GenerateNaive(const ChunkData& chunk_data, std::vector<ChunkVertex>& vertices,
                     std::vector<uint32_t>& indices);
  void GetIndexPadded(const glm::ivec3& pos);
  void GetIndexPadded(int x, int y, int z);
  const BlockDB& block_db_;

  void GenerateBlock(std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices,
                     BlockType block);

 private:
  void AddQuad(int face_idx, const glm::ivec3& block_pos, std::vector<ChunkVertex>& vertices,
               std::vector<uint32_t>& indices, BlockType block);
};
