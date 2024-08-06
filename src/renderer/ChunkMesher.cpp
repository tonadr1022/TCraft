#include "ChunkMesher.hpp"

#include "../gameplay/world/BlockDB.hpp"
#include "../gameplay/world/ChunkData.hpp"
#include "Mesh.hpp"
#include "gameplay/world/ChunkDef.hpp"
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

uint32_t GetVertexData1(uint8_t x, uint8_t y, uint8_t z, uint8_t ao, uint8_t u, uint8_t v) {
  return (x | y << 6 | z << 12 | ao << 18 | u << 20 | v << 26);
}

uint32_t GetVertexData2(uint8_t tex_idx) { return tex_idx; }

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
  x = (x >= ChunkLength) - (x < 0);
  y = (y >= ChunkLength) - (y < 0);
  z = (z >= ChunkLength) - (z < 0);
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
              // AddQuad(face_idx, x, y, z, vertices, indices,
              //         db_mesh_data[block].texture_indices[face_idx]);
              AddQuad(face_idx, x, y, z, vertices, indices, 0);
            }
          } else {
            if (mesh_chunk_blocks[chunk::GetIndex(nx, ny, nz)] == 0) {
              AddQuad(face_idx, x, y, z, vertices, indices, 0);
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

void ChunkMesher::GenerateGreedy(const ChunkNeighborArray& chunks,
                                 std::vector<ChunkVertex>& vertices,
                                 std::vector<uint32_t>& indices) {
  Timer timer;
  auto get_block = [&chunks](int x, int y, int z) -> BlockType {
    if (ChunkData::IsOutOfBounds(x, y, z)) {
      return (*chunks[PosInChunkMeshToChunkNeighborOffset(x, y, z)])[chunk::GetIndex(
          (x + ChunkLength) % ChunkLength, (y + ChunkLength) % ChunkLength,
          (z + ChunkLength) % ChunkLength)];
    }
    return (*chunks[13])[chunk::GetIndex(x, y, z)];
  };
  int u, v, counter, j, i, k, l, height, width;
  int face_num;
  int x[3];  // start point
  int q[3];  // offset
  int du[3];
  int dv[3];
  BlockType block_mask[ChunkArea];

  for (bool back_face = true, b = false; b != back_face; back_face = back_face && b, b = !b) {
    for (int axis = 0; axis < 3; axis++) {
      face_num = ((axis << 1) | (back_face));

      u = (axis + 1) % 3;
      v = (axis + 2) % 3;

      x[0] = 0, x[1] = 0, x[2] = 0, x[axis] = -1;
      q[0] = 0, q[1] = 0, q[2] = 0, q[axis] = 1;

      // move through chunk from front to back
      while (x[axis] < ChunkLength) {
        counter = 0;
        for (x[v] = 0; x[v] < ChunkLength; x[v]++) {
          for (x[u] = 0; x[u] < ChunkLength; x[u]++, counter++) {
            const BlockType block1 = get_block(x[0], x[1], x[2]);
            const BlockType block2 = get_block(x[0] + q[0], x[1] + q[1], x[2] + q[2]);

            if (!back_face) {
              if (!(x[axis] < 0) && ShouldShowFace(block1, block2)) {
                block_mask[counter] = block1;
                // infoMask[counter] = &faceInfo[XYZ(x[0], x[1], x[2])][axis << 1];
              } else {
                // infoMask[counter] = nullptr;
                block_mask[counter] = 0;
              }
            } else {
              if (!(ChunkLengthM1 <= x[axis]) && ShouldShowFace(block2, block1)) {
                block_mask[counter] = block2;
                // infoMask[counter] =
                //     &faceInfo[XYZ(x[0] + q[0], x[1] + q[1], x[2] + q[2])][(axis << 1) | 1];
              } else {
                block_mask[counter] = 0;
                // infoMask[counter] = nullptr;
              }
            }
          }
        }
        x[axis]++;

        // generate mesh for the mask
        counter = 0;

        for (j = 0; j < ChunkLength; j++) {
          for (i = 0; i < ChunkLength;) {
            if (block_mask[counter] != 0) {  // skip if air or zeroed
              // FaceInfo &currFaceInfo = *infoMask[counter];
              // compute width
              for (width = 1;
                   block_mask[counter] == block_mask[counter + width] && i + width < ChunkLength;
                   width++) {
              }

              // compute height
              bool done = false;
              for (height = 1; j + height < ChunkLength; height++) {
                for (k = 0; k < width; k++) {
                  int index = counter + k + height * ChunkLength;
                  if (block_mask[counter] != block_mask[index]) {
                    done = true;
                    break;
                  }
                }
                if (done) break;
              }
              x[u] = i;
              x[v] = j;

              du[0] = 0, du[1] = 0, du[2] = 0;
              dv[0] = 0, dv[1] = 0, dv[2] = 0;

              if (!back_face) {
                dv[v] = height;
                du[u] = width;
              } else {
                du[v] = height;
                dv[u] = width;
              }
              int tex_idx = db_mesh_data[block_mask[counter]].texture_indices[face_num];
              int vx = x[0];
              int vy = x[1];
              int vz = x[2];

              int v00u = du[u] + dv[u];
              int v00v = du[v] + dv[v];
              int v01u = dv[u];
              int v01v = dv[v];
              int v10u = 0;
              int v10v = 0;
              int v11u = du[u];
              int v11v = du[v];

              uint32_t v_data2 = GetVertexData2(tex_idx);
              uint32_t v00_data1 = GetVertexData1(vx, vy, vz, 0, v00u, v00v);
              uint32_t v01_data1 =
                  GetVertexData1(vx + du[0], vy + du[1], vz + du[2], 0, v01u, v01v);
              uint32_t v10_data1 = GetVertexData1(vx + du[0] + dv[0], vy + du[1] + dv[1],
                                                  vz + du[2] + dv[2], 0, v10u, v10v);
              uint32_t v11_data1 =
                  GetVertexData1(vx + dv[0], vy + dv[1], vz + dv[2], 0, v11u, v11v);

              size_t base_vertex_idx = vertices.size();
              vertices.emplace_back(v00_data1, v_data2);
              vertices.emplace_back(v01_data1, v_data2);
              vertices.emplace_back(v10_data1, v_data2);
              vertices.emplace_back(v11_data1, v_data2);

              indices.push_back(base_vertex_idx);
              indices.push_back(base_vertex_idx + 1);
              indices.push_back(base_vertex_idx + 2);
              indices.push_back(base_vertex_idx + 2);
              indices.push_back(base_vertex_idx + 1);
              indices.push_back(base_vertex_idx + 3);

              // zero out the mask for what we just added
              for (l = 0; l < height; l++) {
                for (k = 0; k < width; k++) {
                  size_t index = counter + k + l * ChunkLength;
                  // infoMask[index] = nullptr;
                  block_mask[index] = 0;
                }
              }
              i += width;
              counter += width;
            } else {
              i++;
              counter++;
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
bool ChunkMesher::ShouldShowFace(BlockType curr_block, BlockType compare_block) {
  return curr_block != compare_block;
}
