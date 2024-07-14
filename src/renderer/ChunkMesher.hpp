#pragma once

#include <glm/fwd.hpp>

struct ChunkVertex;
class ChunkData;

class ChunkMesher {
 public:
  static void GenerateNaive(const ChunkData& chunk_data, std::vector<ChunkVertex>& vertices,
                            std::vector<uint32_t>& indices);
  static void GetIndexPadded(const glm::ivec3& pos);
  static void GetIndexPadded(int x, int y, int z);
};
