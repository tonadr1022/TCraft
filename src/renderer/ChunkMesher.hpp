#pragma once

#include <glm/fwd.hpp>

#include "gameplay/world/ChunkData.hpp"

struct ChunkVertex;
class ChunkData;
struct BlockData;
struct BlockMeshData;

class ChunkMesher {
 public:
  ChunkMesher(const std::vector<BlockData>& db_block_data,
              const std::vector<BlockMeshData>& db_mesh_data);
  void GenerateNaive(const ChunkArray& blocks, std::vector<ChunkVertex>& vertices,
                     std::vector<uint32_t>& indices);
  void GenerateSmart(const std::array<ChunkArray*, 27>& chunks, std::vector<ChunkVertex>& vertices,
                     std::vector<uint32_t>& indices);

  // void GenerateGreedy(const ChunkData& chunk_data, std::vector<ChunkVertex>& vertices,
  //                     std::vector<uint32_t>& indices);

  static void GenerateBlock(std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices,
                            const std::array<uint32_t, 6>& tex_indices);

  const std::vector<BlockData>& db_block_data;
  const std::vector<BlockMeshData>& db_mesh_data;

 private:
  static void AddQuad(uint8_t face_idx, uint8_t x, uint8_t y, uint8_t z,
                      std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices,
                      uint32_t tex_idx);
};

class BlockMesher {
 public:
};
