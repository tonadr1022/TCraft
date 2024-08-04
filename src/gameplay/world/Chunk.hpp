#pragma once

#include "ChunkData.hpp"
#include "renderer/Mesh.hpp"

struct ChunkMeshTask {
  std::vector<ChunkVertex> vertices;
  std::vector<uint32_t> indices;
};

class Chunk {
 public:
  explicit Chunk(const glm::ivec3& pos);

  [[nodiscard]] const glm::ivec3& GetPos() const;
  ChunkData& GetData();
  [[nodiscard]] BlockType GetBlock(const glm::ivec3& pos) const;
  [[nodiscard]] const Mesh& GetMesh() const;

 private:
  friend class ChunkManager;
  ChunkData data_;
  Mesh mesh_;
  glm::ivec3 pos_;
};
