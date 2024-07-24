#pragma once

#include "ChunkData.hpp"
#include "renderer/ChunkMesh.hpp"

class Chunk {
 public:
  explicit Chunk(const glm::ivec3& pos);

  [[nodiscard]] const glm::ivec3& GetPos() const;
  ChunkData& GetData();
  ChunkMesh& GetMesh();

 private:
  ChunkData data_;
  ChunkMesh mesh_;
  glm::ivec3 pos_;
};
