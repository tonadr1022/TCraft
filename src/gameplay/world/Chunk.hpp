#pragma once

#include "ChunkData.hpp"
#include "renderer/Mesh.hpp"

struct ChunkMeshTask {
  std::vector<ChunkVertex> vertices;
  std::vector<uint32_t> indices;
  glm::ivec3 pos;
};

class Chunk {
 public:
  explicit Chunk(const glm::ivec3& pos);
  [[nodiscard]] const glm::ivec3& GetPos() const;

  ChunkData data;
  Mesh mesh;

  enum class State { NotFinished, Queued, Finished };
  State terrain_state{State::NotFinished};
  State mesh_state{State::NotFinished};

 private:
  glm::ivec3 pos_;
};
