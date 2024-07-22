#pragma once

#include "ChunkData.hpp"
#include "renderer/ChunkMesh.hpp"

class Chunk {
 public:
  [[nodiscard]] const ChunkArray& GetBlocks() const { return data.GetBlocks(); }
  ChunkData data;
  ChunkMesh mesh;

  bool meshed_{false};

 private:
};
