#include "ChunkMesher.hpp"

#include "../gameplay/world/BlockDB.hpp"
#include "../gameplay/world/ChunkData.hpp"
#include "Mesh.hpp"
#include "gameplay/world/ChunkHelpers.hpp"
#include "util/Timer.hpp"

ChunkMesher::ChunkMesher(const std::vector<BlockData>& db_block_data,
                         const std::vector<BlockMeshData>& db_mesh_data)
    : db_block_data(db_block_data), db_mesh_data(db_mesh_data) {}

namespace {

/* clang-format off */
constexpr int VertexLookup[120] = {
    // front (pos x)
    1, 0, 0, 0, 0,  // bottom left
    1, 1, 0, 0, 1,  // bottom right
    1, 0, 1, 1, 0,  // top left
    1, 1, 1, 1, 1,  // top right
    // back (neg x)
    0, 0, 0, 0, 0, 
    0, 0, 1, 1, 0, 
    0, 1, 0, 0, 1, 
    0, 1, 1, 1, 1,
    // right (pos y)
    0, 1, 0, 0, 0, 
    0, 1, 1, 0, 1, 
    1, 1, 0, 1, 0, 
    1, 1, 1, 1, 1,
    // left (neg y)
    0, 0, 0, 0, 0, 
    1, 0, 0, 1, 0, 
    0, 0, 1, 0, 1, 
    1, 0, 1, 1, 1,
    // top (pos z)
    0, 0, 1, 0, 0, 
    1, 0, 1, 1, 0, 
    0, 1, 1, 0, 1, 
    1, 1, 1, 1, 1,
    // bottom (neg z)
    0, 0, 0, 0, 0, 
    0, 1, 0, 0, 1, 
    1, 0, 0, 1, 0, 
    1, 1, 0, 1, 1,
};
/* clang-format on */

uint32_t GetVertexData1(uint8_t x, uint8_t y, uint8_t z, uint8_t u, uint8_t v, uint8_t tex_idx) {
  return (x | y << 6 | z << 12 | u << 18 | v << 19 | tex_idx << 20);
}

// uint32_t GetVertexData2(uint8_t x, uint8_t y, uint8_t z) { return (x | y << 6 | z << 12); }

}  // namespace

void ChunkMesher::AddQuad(uint8_t face_idx, uint8_t x, uint8_t y, uint8_t z,
                          std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices,
                          uint32_t tex_idx) {
  uint32_t base_vertex_idx = vertices.size();
  for (uint8_t vertex_idx = 0, lookup_offset = 0; vertex_idx < 4;
       vertex_idx++, lookup_offset += 5) {
    int combined_offset = face_idx * 20 + lookup_offset;
    vertices.emplace_back(
        GetVertexData1(x + VertexLookup[combined_offset], y + VertexLookup[combined_offset + 1],
                       z + VertexLookup[combined_offset + 2], VertexLookup[combined_offset + 3],
                       VertexLookup[combined_offset + 4], tex_idx),
        0);
  }

  // // check whether to flip quad based on AO
  // if (occlusionLevels[0] + occlusionLevels[3] > occlusionLevels[1] + occlusionLevels[2]) {
  //   indices.push_back(baseVertexIndex + 2);
  //   indices.push_back(baseVertexIndex + 0);
  //   indices.push_back(baseVertexIndex + 3);
  //   indices.push_back(baseVertexIndex + 3);
  //   indices.push_back(baseVertexIndex + 0);
  //   indices.push_back(baseVertexIndex + 1);
  //
  // } else {
  //   indices.push_back(baseVertexIndex);
  //   indices.push_back(baseVertexIndex + 1);
  //   indices.push_back(baseVertexIndex + 2);
  //   indices.push_back(baseVertexIndex + 2);
  //   indices.push_back(baseVertexIndex + 1);
  //   indices.push_back(baseVertexIndex + 3);
  // }

  indices.push_back(base_vertex_idx);
  indices.push_back(base_vertex_idx + 1);
  indices.push_back(base_vertex_idx + 2);
  indices.push_back(base_vertex_idx + 2);
  indices.push_back(base_vertex_idx + 1);
  indices.push_back(base_vertex_idx + 3);
}

void ChunkMesher::GenerateBlock(std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices,
                                const std::array<uint32_t, 6>& tex_indices) {
  for (int face_idx = 0; face_idx < 6; face_idx++) {
    AddQuad(face_idx, 0, 0, 0, vertices, indices, tex_indices[face_idx]);
  }
}

void ChunkMesher::GenerateNaive(const BlockTypeArray& blocks, std::vector<ChunkVertex>& vertices,
                                std::vector<uint32_t>& indices) {
  ZoneScoped;
  auto get_block_type = [&blocks](int x, int y, int z) -> BlockType {
    if (ChunkData::IsOutOfBounds(x, y, z)) return 0;
    return blocks[ChunkData::GetIndex(x, y, z)];
  };

  int idx = 0;
  for (int y = 0; y < ChunkLength; y++) {
    for (int z = 0; z < ChunkLength; z++) {
      for (int x = 0; x < ChunkLength; x++, idx++) {
        ZoneScopedN("for loop iter");
        BlockType block = blocks[idx];
        if (block == 0) continue;
        static constexpr const int Offsets[6][3] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                                                    {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
        for (int face_idx = 0; face_idx < 6; face_idx++) {
          ZoneScopedN("Face idx");
          int nx = x + Offsets[face_idx][0];
          int ny = y + Offsets[face_idx][1];
          int nz = z + Offsets[face_idx][2];
          {
            if (get_block_type(nx, ny, nz) == 0) {
              AddQuad(face_idx, x, y, z, vertices, indices,
                      db_mesh_data[static_cast<uint32_t>(block)].texture_indices[face_idx]);
            }
          }
        }
      }
    }
  }
}

namespace {
uint8_t PosInChunkMeshToChunkNeighborOffset(int x, int y, int z) {
  if (x >= ChunkLength)
    x = 1;
  else if (x < 0)
    x = -1;
  else
    x = 0;
  if (y >= ChunkLength)
    y = 1;
  else if (y < 0)
    y = -1;
  else
    y = 0;
  if (z >= ChunkLength)
    z = 1;
  else if (z < 0)
    z = -1;
  else
    z = 0;

  return ChunkNeighborOffsetToIdx(x, y, z);
}
constexpr const int Offsets[6][3] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                                     {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
}  // namespace

void ChunkMesher::GenerateSmart(const ChunkNeighborArray& chunks,
                                std::vector<ChunkVertex>& vertices,
                                std::vector<uint32_t>& indices) {
  Timer timer;
  int idx = 0;
  int nx;
  int ny;
  int nz;
  int x;
  int y;
  int z;
  int face_idx;
  const BlockTypeArray& mesh_chunk_blocks = *chunks[13];
  for (y = 0; y < ChunkLength; y++) {
    for (z = 0; z < ChunkLength; z++) {
      for (x = 0; x < ChunkLength; x++, idx++) {
        BlockType block = mesh_chunk_blocks[idx];
        if (block == 0) continue;
        for (face_idx = 0; face_idx < 6; face_idx++) {
          nx = x + Offsets[face_idx][0];
          ny = y + Offsets[face_idx][1];
          nz = z + Offsets[face_idx][2];
          if (ChunkData::IsOutOfBounds(nx, ny, nz)) {
            if ((*chunks[PosInChunkMeshToChunkNeighborOffset(nx, ny, nz)])[chunk::GetIndex(
                    (nx + ChunkLength) % ChunkLength, (ny + ChunkLength) % ChunkLength,
                    (nz + ChunkLength) % ChunkLength)] == 0) {
              AddQuad(face_idx, x, y, z, vertices, indices,
                      db_mesh_data[block].texture_indices[face_idx]);
            }
          } else {
            if ((*chunks[13])[chunk::GetIndex(nx, ny, nz)] == 0) {
              AddQuad(face_idx, x, y, z, vertices, indices,
                      db_mesh_data[static_cast<uint32_t>(block)].texture_indices[face_idx]);
            }
          }
        }
      }
    }
  }
  static double total = 0;
  static int count = 0;
  count++;
  double curr = timer.ElapsedMS();
  total += curr;
  spdlog::info("{} {}", curr, total / count);
}

// void ChunkMesher::GenerateGreedy(const ChunkData& chunk_data, std::vector<ChunkVertex>&
// vertices,
//                                  std::vector<uint32_t>& indices) {}
