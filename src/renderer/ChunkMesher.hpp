#pragma once

#include <glm/fwd.hpp>

struct ChunkVertex;
class ChunkData;
class BlockData;
class BlockMeshData;

class ChunkMesher {
 public:
  ChunkMesher(const std::vector<BlockData>& db_block_data,
              const std::vector<BlockMeshData>& db_mesh_data);
  void GenerateNaive(const ChunkData& chunk_data, std::vector<ChunkVertex>& vertices,
                     std::vector<uint32_t>& indices);
  void GetIndexPadded(const glm::ivec3& pos);
  void GetIndexPadded(int x, int y, int z);

  static void GenerateBlock(std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices,
                            const std::array<uint32_t, 6>& tex_indices);

  const std::vector<BlockData>& db_block_data;
  const std::vector<BlockMeshData>& db_mesh_data;

 private:
  static void AddQuad(int face_idx, const glm::ivec3& block_pos, std::vector<ChunkVertex>& vertices,
                      std::vector<uint32_t>& indices, int tex_idx);
};

class BlockMesher {
 public:
};
