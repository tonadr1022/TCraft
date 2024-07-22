#include "ChunkMesher.hpp"

#include "../gameplay/world/BlockDB.hpp"
#include "../gameplay/world/ChunkData.hpp"
#include "ChunkMesh.hpp"
#include "gameplay/world/Block.hpp"

namespace {

/* clang-format off */
constexpr int VertexLookup[120] = {
    // front (pos x)
    1, 0, 0, 0, 0,  // bottom left
    1, 1, 0, 1, 0,  // bottom right
    1, 0, 1, 0, 1,  // top left
    1, 1, 1, 1, 1,  // top right
    // back (neg x)
    0, 0, 0, 0, 0, 
    0, 0, 1, 0, 1, 
    0, 1, 0, 1, 0, 
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

}  // namespace

void ChunkMesher::AddQuad(int face_idx, const glm::ivec3& block_pos,
                          std::vector<ChunkVertex>& vertices, std::vector<uint32_t>& indices,
                          BlockType block) {
  int base_vertex_idx = vertices.size();
  face_idx *= 20;
  for (int vertex_idx = 0, lookup_offset = 0; vertex_idx < 4; vertex_idx++, lookup_offset += 5) {
    ChunkVertex vertex;
    int combined_offset = face_idx + lookup_offset;
    vertex.position.x = block_pos.x + VertexLookup[combined_offset];
    vertex.position.y = block_pos.y + VertexLookup[combined_offset + 1];
    vertex.position.z = block_pos.z + VertexLookup[combined_offset + 2];
    vertex.tex_coords.s = VertexLookup[combined_offset + 3];
    vertex.tex_coords.t = VertexLookup[combined_offset + 4];
    vertex.index = block_db_.GetMeshData()[static_cast<int>(block)].texture_indices[vertex_idx];
    vertices.emplace_back(vertex);
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

void ChunkMesher::GenerateNaive(const ChunkData& chunk_data, std::vector<ChunkVertex>& vertices,
                                std::vector<uint32_t>& indices) {
  ZoneScoped;
  const auto& blocks = chunk_data.GetBlocks();
  glm::ivec3 pos;
  int idx = 0;
  for (int y = 0; y < ChunkLength; y++) {
    for (int z = 0; z < ChunkLength; z++) {
      for (int x = 0; x < ChunkLength; x++, idx++) {
        BlockType block = blocks[ChunkData::GetIndex(x, y, z)];
        if (block == BlockType::Air) continue;
        // 1, 0, 0
        // -1, 0, 0
        // 0, 1, 0
        // 0, -1, 0
        // 0, 0, 1
        // 0, 0, -1
        for (int face_idx = 0; face_idx < 6; face_idx++) {
          // get adj block position
          int adj_block_pos_arr[3] = {x, y, z};
          adj_block_pos_arr[face_idx >> 1] += 1 - ((face_idx & 1) << 1);
          glm::ivec3 adj_pos = {adj_block_pos_arr[0], adj_block_pos_arr[1], adj_block_pos_arr[2]};
          if (ChunkData::IsOutOfBounds(adj_pos) ||
              blocks[ChunkData::GetIndex(adj_pos)] == BlockType::Air) {
            AddQuad(face_idx, {x, y, z}, vertices, indices, block);
          }
        }
      }
    }
  }
}
