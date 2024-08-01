#pragma once

#include "ChunkData.hpp"
#include "renderer/Mesh.hpp"

class Chunk {
 public:
  explicit Chunk(const glm::ivec3& pos);

  [[nodiscard]] const glm::ivec3& GetPos() const;
  ChunkData& GetData();
  Mesh& GetMesh();

 private:
  ChunkData data_;
  Mesh mesh_;
  glm::ivec3 pos_;
};
