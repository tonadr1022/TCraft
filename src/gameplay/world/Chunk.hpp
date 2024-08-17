#pragma once

#include "ChunkData.hpp"

enum class LODLevel { NoMesh, Regular, One, Two, Three };
struct ChunkMeshTask {
  std::vector<ChunkVertex> vertices;
  std::vector<uint32_t> indices;
  glm::ivec3 pos;
  LODLevel lod_level;
};

class Chunk {
 public:
  explicit Chunk(const glm::ivec3& pos);
  [[nodiscard]] const glm::ivec3& GetPos() const;

  ChunkData data;
  uint32_t mesh_handle{0};
  LODLevel lod_level{LODLevel::NoMesh};

  enum class State { NotFinished, Queued, Finished };
  State terrain_state{State::NotFinished};
  State mesh_state{State::NotFinished};

 private:
  glm::ivec3 pos_;
};
