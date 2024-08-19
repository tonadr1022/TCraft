#include "ChunkMesher.hpp"

#include <cstddef>

#include "../gameplay/world/BlockDB.hpp"
#include "../gameplay/world/ChunkData.hpp"
#include "gameplay/world/Chunk.hpp"
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

uint32_t GetLODVertexData1(int x, int y, int z) { return x | y << 10 | z << 20; }
uint32_t GetLODVertexData2(int u, int v, uint32_t tex_idx) { return (u | v << 10 | tex_idx << 20); }

uint32_t GetVertexData1(uint8_t x, uint8_t y, uint8_t z, uint8_t u, uint8_t v, uint8_t ao) {
  return (x | y << 6 | z << 12 | ao << 18 | u << 20 | v << 26);
}

uint32_t GetVertexData2(uint32_t tex_idx) { return tex_idx; }

struct FaceInfo {
  struct Ao {
    unsigned v0 : 2;
    unsigned v1 : 2;
    unsigned v2 : 2;
    unsigned v3 : 2;
    bool operator==(const Ao& other) const {
      return v0 == other.v0 && v1 == other.v1 && v2 == other.v2 && v3 == other.v3;
    }

    bool operator!=(const Ao& other) const { return !(*this == other); }
  };

  Ao ao;
  bool initialized{false};
  [[nodiscard]] bool Flip() const { return ao.v0 + ao.v2 > ao.v1 + ao.v3; }

  bool operator==(const FaceInfo& other) const { return ao == other.ao; }

  bool operator!=(const FaceInfo& other) const {
    return ao.v0 != other.ao.v0 || ao.v1 != other.ao.v1 || ao.v2 != other.ao.v2 ||
           ao.v3 != other.ao.v3;
  }
  // FaceLighting (for meshing)
  void SetValues(int face_num, const BlockType (&block_neighbors)[27]) {
    if (initialized) return;
    initialized = true;
    // y
    // |
    // |  6   15  24
    // |    7   16  25
    // |      8   17  26
    // |
    // |  3   12  21
    // |    4   13  22
    // |      5   14  23
    // \-------------------x
    //  \ 0   9   18
    //   \  1   10  19
    //    \   2   11  20
    //     z

    constexpr const int Lookup3[6][4][3] = {
        {{21, 18, 19}, {21, 24, 25}, {23, 26, 25}, {23, 20, 19}},
        {{3, 0, 1}, {5, 2, 1}, {5, 8, 7}, {3, 6, 7}},
        {{15, 6, 7}, {17, 8, 7}, {17, 26, 25}, {15, 24, 25}},
        {{9, 0, 1}, {9, 18, 19}, {11, 20, 19}, {11, 2, 1}},
        {{11, 2, 5}, {11, 20, 23}, {17, 26, 23}, {17, 8, 5}},
        {{9, 0, 3}, {15, 6, 3}, {15, 24, 21}, {9, 18, 21}}};
    // constexpr const int Lookup1[6] = {22, 4, 16, 10, 14, 12};

    uint8_t sides[3];
    bool trans[3];

    for (int v = 0; v < 4; ++v) {
      for (int i = 0; i < 3; ++i) {
        sides[i] = block_neighbors[Lookup3[face_num][v][i]];
        // TODO: transparency
        trans[i] = sides[i] == 0;
        // canPass[i] = BlockMethods::LightCanPass(sides[i]);
      }
      uint8_t val = (!trans[0] && !trans[2] ? 0 : 3 - !trans[0] - !trans[1] - !trans[2]);
      switch (v) {
        case 0:
          ao.v0 = val;
          break;
        case 1:
          ao.v1 = val;
          break;
        case 2:
          ao.v2 = val;
          break;
        case 3:
          ao.v3 = val;
          break;
      }

      //   // smooth the Light using the average value
      //   uint8_t counter = 1, sunLightSum = sunlight_neighbours[Lookup1[face]],
      //           torchLightSum = torchlight_neighbours[Lookup1[face]];
      //   if (canPass[0] || canPass[2])
      //     for (int i = 0; i < 3; ++i) {
      //       if (!canPass[i]) continue;
      //       counter++;
      //       sunLightSum += sunlight_neighbours[Lookup3[face][v][i]];
      //       torchLightSum += torchlight_neighbours[Lookup3[face][v][i]];
      //     }
      //
      //   this->sunlight[v] = sunLightSum / counter;
      //   this->torchlight[v] = torchLightSum / counter;
      // }
    }
  }
};

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
                       VertexLookup[combined_offset + 4], 0),
        GetVertexData2(tex_idx));
  }

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

namespace {

uint8_t PosInChunkMeshToChunkNeighborOffset(int x, int y, int z) {
  x = (x >= ChunkLength) - (x < 0);
  y = (y >= ChunkLength) - (y < 0);
  z = (z >= ChunkLength) - (z < 0);
  return ChunkNeighborOffsetToIdx(x, y, z);
}
}  // namespace

bool ChunkMesher::ShouldShowFace(BlockType curr_block, BlockType compare_block) {
  // TODO: transparency
  bool trans = curr_block == 0, trans_neighbour = compare_block == 0;
  if (curr_block == 0) return false;
  if (!trans && !trans_neighbour) return false;
  if (trans && trans_neighbour && curr_block != compare_block) return true;
  return !(trans && compare_block);
}

namespace {
constexpr const int NeighborOffsets[6][3] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                                             {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
}
void ChunkMesher::GenerateSmart(const ChunkNeighborArray& chunks,
                                std::vector<ChunkVertex>& vertices,
                                std::vector<uint32_t>& indices) const {
  Timer timer;
  int idx = 0;
  int nx;
  int ny;
  int nz;
  int x;
  int y;
  int z;
  int face_idx;
  const BlockTypeArray& mesh_chunk_blocks = *chunks[13]->data.blocks_;
  for (y = 0; y < ChunkLength; y++) {
    for (z = 0; z < ChunkLength; z++) {
      for (x = 0; x < ChunkLength; x++, idx++) {
        BlockType block = mesh_chunk_blocks[idx];
        if (block == 0) continue;
        for (face_idx = 0; face_idx < 6; face_idx++) {
          nx = x + NeighborOffsets[face_idx][0];
          ny = y + NeighborOffsets[face_idx][1];
          nz = z + NeighborOffsets[face_idx][2];
          if (ChunkData::IsOutOfBounds(nx, ny, nz)) {
            if ((*chunks[PosInChunkMeshToChunkNeighborOffset(nx, ny, nz)])
                    .data.GetBlock((nx + ChunkLength) % ChunkLength,
                                   (ny + ChunkLength) % ChunkLength,
                                   (nz + ChunkLength) % ChunkLength) == 0) {
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

void ChunkMesher::GenerateLODGreedy(const ChunkData& chunk_data, std::vector<ChunkVertex>& vertices,
                                    std::vector<uint32_t>& indices) {
  ZoneScoped;
  if (chunk_data.GetBlockCount() == 0) return;
  // TODO: make parameter
  // std::array<BlockType, ChunkVolume / 8> blocks;
  const auto& blocks = *chunk_data.blocks_lod_1_;
  uint32_t f = 2;
  int chunk_length = ChunkLength / f;
  auto get_index = [chunk_length](int x, int y, int z) -> int {
    return x + z * chunk_length + y * chunk_length * chunk_length;
  };

  for (std::size_t axis = 0; axis < 3; ++axis) {
    const std::size_t u = (axis + 1) % 3;
    const std::size_t v = (axis + 2) % 3;

    // TODO: make parameter
    int x[3] = {0}, q[3] = {0}, block_mask[ChunkArea / 4];

    // Compute mask
    q[axis] = 1;
    for (x[axis] = -1; x[axis] < chunk_length;) {
      std::size_t counter = 0;
      for (x[v] = 0; x[v] < chunk_length; ++x[v]) {
        for (x[u] = 0; x[u] < chunk_length; ++x[u], ++counter) {
          bool block1_oob = x[axis] < 0, block2_oob = chunk_length - 1 <= x[axis];
          const BlockType block1 = block1_oob ? 0 : blocks[get_index(x[0], x[1], x[2])];
          const BlockType block2 =
              block2_oob ? 0 : blocks[get_index(x[0] + q[0], x[1] + q[1], x[2] + q[2])];

          if (!block1_oob && ShouldShowFace(block1, block2)) {
            block_mask[counter] = block1;
          } else if (!block2_oob && ShouldShowFace(block2, block1)) {
            block_mask[counter] = -block2;
          } else {
            block_mask[counter] = 0;
          }
        }
      }

      ++x[axis];

      // Generate mesh for mask using lexicographic ordering
      uint32_t width = 0, height = 0;

      counter = 0;
      for (uint32_t j = 0; j < static_cast<uint32_t>(chunk_length); ++j) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(chunk_length);) {
          int quad_type = block_mask[counter];
          if (quad_type) {
            // Compute width
            for (width = 1; quad_type == block_mask[counter + width] &&
                            i + width < static_cast<uint32_t>(chunk_length);
                 ++width);

            // Compute height
            bool done = false;
            for (height = 1; j + height < static_cast<uint32_t>(chunk_length); ++height) {
              for (uint32_t k = 0; k < width; ++k) {
                uint32_t ind = counter + k + height * chunk_length;
                if (quad_type != block_mask[ind]) {
                  done = true;
                  break;
                }
              }

              if (done) break;
            }

            // Add quad
            x[u] = i;
            x[v] = j;

            int du[3] = {0}, dv[3] = {0};

            int quad_face = ((axis << 1) | (quad_type <= 0));

            if (quad_type > 0) {
              dv[v] = height * 2;
              du[u] = width * 2;
            } else {
              quad_type = -quad_type;
              du[v] = height * 2;
              dv[u] = width * 2;
            }

            uint32_t tex_idx = db_mesh_data[quad_type].texture_indices[quad_face];
            int vx = x[0] * 2;
            int vy = x[1] * 2;
            int vz = x[2] * 2;

            int v00u = du[u] + dv[u];
            int v00v = du[v] + dv[v];
            int v01u = dv[u];
            int v01v = dv[v];
            int v10u = 0;
            int v10v = 0;
            int v11u = du[u];
            int v11v = du[v];

            if (quad_face == 1) {
              std::swap(v00u, v01v);
              std::swap(v00v, v01u);
              std::swap(v11u, v10v);
              std::swap(v11v, v10u);
            } else if (quad_face == 0) {
              std::swap(v11u, v11v);
              std::swap(v01u, v01v);
              std::swap(v00u, v00v);
            } else if (quad_face == 4) {
              std::swap(v00u, v01u);
              std::swap(v00v, v01v);
              std::swap(v11u, v10u);
              std::swap(v11v, v10v);
            }

            uint32_t v_data2 = GetVertexData2(tex_idx);
            uint32_t v00_data1 = GetVertexData1(vx, vy, vz, v00u, v00v, 3);
            uint32_t v01_data1 = GetVertexData1(vx + du[0], vy + du[1], vz + du[2], v01u, v01v, 3);
            uint32_t v10_data1 = GetVertexData1(vx + du[0] + dv[0], vy + du[1] + dv[1],
                                                vz + du[2] + dv[2], v10u, v10v, 3);
            uint32_t v11_data1 = GetVertexData1(vx + dv[0], vy + dv[1], vz + dv[2], v11u, v11v, 3);

            int base_vertex_idx = vertices.size();
            vertices.emplace_back(v00_data1, v_data2);
            vertices.emplace_back(v01_data1, v_data2);
            vertices.emplace_back(v10_data1, v_data2);
            vertices.emplace_back(v11_data1, v_data2);

            indices.push_back(base_vertex_idx + 1);
            indices.push_back(base_vertex_idx + 2);
            indices.push_back(base_vertex_idx + 3);
            indices.push_back(base_vertex_idx);
            indices.push_back(base_vertex_idx + 1);
            indices.push_back(base_vertex_idx + 3);

            for (std::size_t b = 0; b < width; ++b)
              for (std::size_t a = 0; a < height; ++a) {
                size_t ind = counter + b + a * chunk_length;
                block_mask[ind] = 0;
              }

            // Increment counters
            i += width;
            counter += width;
          } else {
            ++i;
            ++counter;
          }
        }
      }
    }
  }
}

void ChunkMesher::GenerateGreedy(const ChunkNeighborArray& chunks,
                                 std::vector<ChunkVertex>& vertices,
                                 std::vector<uint32_t>& indices) {
  // Timer timer;
  BlockTypeArray* mesh_chunk_blocks_ptr = (*chunks[13]).data.GetBlocks();
  if (!mesh_chunk_blocks_ptr) return;
  const BlockTypeArray& mesh_chunk_blocks = *mesh_chunk_blocks_ptr;
  int chunk_length = ChunkLength;
  auto get_block = [chunk_length, &chunks](int x, int y, int z) -> BlockType {
    const std::shared_ptr<Chunk>& c = chunks[PosInChunkMeshToChunkNeighborOffset(x, y, z)];
    if (c == nullptr) return 0;
    return c->data.GetBlock((x + chunk_length) % chunk_length, (y + chunk_length) % chunk_length,
                            (z + chunk_length) % chunk_length);
  };

  std::unordered_map<uint32_t, std::array<FaceInfo, 6>> face_info_map;
  BlockType neighbors[27];
  auto get_face_info = [&face_info_map, &get_block, mesh_chunk_blocks, &neighbors](
                           int x, int y, int z, uint32_t face) -> FaceInfo& {
    uint32_t idx = chunk::GetIndex(x, y, z);
    auto it = face_info_map.find(idx);
    if (it == face_info_map.end()) {
      face_info_map[idx] = std::array<FaceInfo, 6>{};
    }

    if (face_info_map[idx][face].initialized) return face_info_map[idx][face];
    BlockType block = mesh_chunk_blocks[idx];
    if (block == 0) return face_info_map[idx][face];

    bool neighbors_initialized = false;
    for (uint8_t face = 0; face < 6; ++face) {
      int nei[3] = {static_cast<int>(x), static_cast<int>(y), static_cast<int>(z)};
      nei[face >> 1] += 1 - ((face & 1) << 1);
      BlockType block_neighbor = get_block(nei[0], nei[1], nei[2]);

      if (!ShouldShowFace(block, block_neighbor)) continue;

      if (!neighbors_initialized) {
        neighbors_initialized = true;
        int it[3], ind = 0;
        for (it[0] = x - 1; it[0] <= x + 1; ++it[0])
          for (it[1] = y - 1; it[1] <= y + 1; ++it[1])
            for (it[2] = z - 1; it[2] <= z + 1; ++it[2], ++ind) {
              neighbors[ind] = get_block(it[0], it[1], it[2]);
            }
      }
      face_info_map[idx][face].SetValues(face, neighbors);
    }

    return face_info_map[idx][face];
  };

  for (std::size_t axis = 0; axis < 3; ++axis) {
    const std::size_t u = (axis + 1) % 3;
    const std::size_t v = (axis + 2) % 3;

    int x[3] = {0}, q[3] = {0}, block_mask[ChunkArea];
    FaceInfo* info_mask[ChunkArea];

    // Compute mask
    q[axis] = 1;
    for (x[axis] = -1; x[axis] < chunk_length;) {
      std::size_t counter = 0;
      for (x[v] = 0; x[v] < chunk_length; ++x[v]) {
        for (x[u] = 0; x[u] < chunk_length; ++x[u], ++counter) {
          bool block1_oob = x[axis] < 0, block2_oob = chunk_length - 1 <= x[axis];
          const BlockType block1 = block1_oob
                                       ? get_block(x[0], x[1], x[2])
                                       : mesh_chunk_blocks[chunk::GetIndex(x[0], x[1], x[2])];
          const BlockType block2 =
              block2_oob
                  ? get_block(x[0] + q[0], x[1] + q[1], x[2] + q[2])
                  : mesh_chunk_blocks[chunk::GetIndex(x[0] + q[0], x[1] + q[1], x[2] + q[2])];

          if (!block1_oob && ShouldShowFace(block1, block2)) {
            block_mask[counter] = block1;
            info_mask[counter] = &get_face_info(x[0], x[1], x[2], axis << 1);

          } else if (!block2_oob && ShouldShowFace(block2, block1)) {
            block_mask[counter] = -block2;
            info_mask[counter] =
                &get_face_info(x[0] + q[0], x[1] + q[1], x[2] + q[2], axis << 1 | 1);
          } else {
            block_mask[counter] = 0;
            info_mask[counter] = nullptr;
          }
        }
      }

      ++x[axis];

      // Generate mesh for mask using lexicographic ordering
      size_t width = 0, height = 0;

      counter = 0;
      for (size_t j = 0; j < static_cast<size_t>(chunk_length); ++j) {
        for (size_t i = 0; i < static_cast<size_t>(chunk_length);) {
          int quad_type = block_mask[counter];
          if (quad_type) {
            const FaceInfo& curr_face_info = *info_mask[counter];
            // Compute width
            for (width = 1;
                 quad_type == block_mask[counter + width] && info_mask[counter + width] &&
                 curr_face_info == *info_mask[counter + width] &&
                 i + width < static_cast<size_t>(chunk_length);
                 ++width);

            // Compute height
            bool done = false;
            for (height = 1; j + height < static_cast<size_t>(chunk_length); ++height) {
              for (size_t k = 0; k < width; ++k) {
                size_t ind = counter + k + height * chunk_length;
                if (quad_type != block_mask[ind] || !info_mask[ind] ||
                    curr_face_info != *info_mask[ind]) {
                  done = true;
                  break;
                }
              }

              if (done) break;
            }

            // Add quad
            x[u] = i;
            x[v] = j;

            int du[3] = {0}, dv[3] = {0};

            int quad_face = ((axis << 1) | (quad_type <= 0));

            if (quad_type > 0) {
              dv[v] = height;
              du[u] = width;
            } else {
              quad_type = -quad_type;
              du[v] = height;
              dv[u] = width;
            }

            uint32_t tex_idx = db_mesh_data[quad_type].texture_indices[quad_face];
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

            if (quad_face == 1) {
              std::swap(v00u, v01v);
              std::swap(v00v, v01u);
              std::swap(v11u, v10v);
              std::swap(v11v, v10u);
            } else if (quad_face == 0) {
              std::swap(v11u, v11v);
              std::swap(v01u, v01v);
              std::swap(v00u, v00v);
            } else if (quad_face == 4) {
              std::swap(v00u, v01u);
              std::swap(v00v, v01v);
              std::swap(v11u, v10u);
              std::swap(v11v, v10v);
            }

            uint32_t v_data2 = GetVertexData2(tex_idx);
            uint32_t v00_data1 = GetVertexData1(vx, vy, vz, v00u, v00v, curr_face_info.ao.v0);
            uint32_t v01_data1 = GetVertexData1(vx + du[0], vy + du[1], vz + du[2], v01u, v01v,
                                                curr_face_info.ao.v1);
            uint32_t v10_data1 =
                GetVertexData1(vx + du[0] + dv[0], vy + du[1] + dv[1], vz + du[2] + dv[2], v10u,
                               v10v, curr_face_info.ao.v2);
            uint32_t v11_data1 = GetVertexData1(vx + dv[0], vy + dv[1], vz + dv[2], v11u, v11v,
                                                curr_face_info.ao.v3);
            int base_vertex_idx = vertices.size();
            vertices.emplace_back(v00_data1, v_data2);
            vertices.emplace_back(v01_data1, v_data2);
            vertices.emplace_back(v10_data1, v_data2);
            vertices.emplace_back(v11_data1, v_data2);

            if (curr_face_info.Flip()) {
              indices.push_back(base_vertex_idx + 0);
              indices.push_back(base_vertex_idx + 1);
              indices.push_back(base_vertex_idx + 2);
              indices.push_back(base_vertex_idx);
              indices.push_back(base_vertex_idx + 2);
              indices.push_back(base_vertex_idx + 3);
            } else {
              indices.push_back(base_vertex_idx + 1);
              indices.push_back(base_vertex_idx + 2);
              indices.push_back(base_vertex_idx + 3);
              indices.push_back(base_vertex_idx);
              indices.push_back(base_vertex_idx + 1);
              indices.push_back(base_vertex_idx + 3);
            }

            for (std::size_t b = 0; b < width; ++b)
              for (std::size_t a = 0; a < height; ++a) {
                size_t ind = counter + b + a * chunk_length;
                block_mask[ind] = 0;
                info_mask[ind] = nullptr;
              }

            // Increment counters
            i += width;
            counter += width;
          } else {
            ++i;
            ++counter;
          }
        }
      }
    }
  }
  // static double total = 0;
  // static int count = 0;
  // count++;
  // double curr = timer.ElapsedMS();
  // total += curr;
  // spdlog::info("{}", total / count);
}

void ChunkMesher::GenerateLODGreedy2(const ChunkStackArray& chunk_data,
                                     std::vector<ChunkVertex>& vertices,
                                     std::vector<uint32_t>& indices) {
  ZoneScoped;
  // TODO: make parameter
  uint32_t f = 2;
  int chunk_length = ChunkLength / f;

  glm::ivec3 dims = {chunk_length, chunk_length * NumVerticalChunks, chunk_length};
  auto get_block = [&chunk_data, &chunk_length](int x, int y, int z) {
    return chunk_data[y / chunk_length]->data.GetBlockLOD1(x, y % chunk_length, z);
  };

  std::vector<int> block_mask(chunk_length * chunk_length * NumVerticalChunks);
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const std::size_t u = (axis + 1) % 3;
    const std::size_t v = (axis + 2) % 3;

    // TODO: make parameter
    int x[3] = {0}, q[3] = {0};

    // Compute mask
    q[axis] = 1;
    for (x[axis] = -1; x[axis] < dims[axis];) {
      std::size_t counter = 0;
      for (x[v] = 0; x[v] < dims[v]; ++x[v]) {
        for (x[u] = 0; x[u] < dims[u]; ++x[u], ++counter) {
          bool block1_oob = x[axis] < 0, block2_oob = dims[axis] - 1 <= x[axis];
          const BlockType block1 = block1_oob ? 0 : get_block(x[0], x[1], x[2]);
          const BlockType block2 =
              block2_oob ? 0 : get_block(x[0] + q[0], x[1] + q[1], x[2] + q[2]);

          if (!block1_oob && ShouldShowFace(block1, block2)) {
            block_mask[counter] = block1;
          } else if (!block2_oob && ShouldShowFace(block2, block1)) {
            block_mask[counter] = -block2;
          } else {
            block_mask[counter] = 0;
          }
        }
      }

      ++x[axis];

      // Generate mesh for mask using lexicographic ordering
      int width = 0, height = 0;

      counter = 0;
      for (int j = 0; j < dims[v]; ++j) {
        for (int i = 0; i < dims[u];) {
          int quad_type = block_mask[counter];
          if (quad_type) {
            // Compute width
            for (width = 1; quad_type == block_mask[counter + width] && i + width < dims[u];
                 ++width);

            // Compute height
            bool done = false;
            for (height = 1; j + height < dims[v]; ++height) {
              for (int k = 0; k < width; ++k) {
                if (quad_type != block_mask[counter + k + height * dims[u]]) {
                  done = true;
                  break;
                }
              }

              if (done) break;
            }

            // Add quad
            x[u] = i;
            x[v] = j;

            int du[3] = {0}, dv[3] = {0};

            if (quad_type > 0) {
              dv[v] = height * 2;
              du[u] = width * 2;
            } else {
              quad_type = -quad_type;
              du[v] = height * 2;
              dv[u] = width * 2;
            }

            int quad_face = ((axis << 1) | (quad_type <= 0));

            uint32_t tex_idx = db_mesh_data[quad_type].texture_indices[quad_face];
            int vx = x[0] * 2;
            int vy = x[1] * 2;
            int vz = x[2] * 2;

            int v00u = du[u] + dv[u];
            int v00v = du[v] + dv[v];
            int v01u = dv[u];
            int v01v = dv[v];
            int v10u = 0;
            int v10v = 0;
            int v11u = du[u];
            int v11v = du[v];

            int base_vertex_idx = vertices.size();
            vertices.emplace_back(GetLODVertexData1(vx, vy, vz),
                                  GetLODVertexData2(v00u, v00v, tex_idx));
            vertices.emplace_back(GetLODVertexData1(vx + du[0], vy + du[1], vz + du[2]),
                                  GetLODVertexData2(v01u, v01v, tex_idx));
            vertices.emplace_back(
                GetLODVertexData1(vx + du[0] + dv[0], vy + du[1] + dv[1], vz + du[2] + dv[2]),
                GetLODVertexData2(v10u, v10v, tex_idx));
            vertices.emplace_back(GetLODVertexData1(vx + dv[0], vy + dv[1], vz + dv[2]),
                                  GetLODVertexData2(v11u, v11v, tex_idx));

            indices.push_back(base_vertex_idx + 1);
            indices.push_back(base_vertex_idx + 2);
            indices.push_back(base_vertex_idx + 3);
            indices.push_back(base_vertex_idx);
            indices.push_back(base_vertex_idx + 1);
            indices.push_back(base_vertex_idx + 3);

            for (int b = 0; b < width; ++b)
              for (int a = 0; a < height; ++a) {
                block_mask[counter + b + a * dims[u]] = 0;
              }

            // Increment counters
            i += width;
            counter += width;
          } else {
            ++i;
            ++counter;
          }
        }
      }
    }
  }
}
