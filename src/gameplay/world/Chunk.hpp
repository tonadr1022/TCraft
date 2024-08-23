#pragma once

#include <glm/vec2.hpp>

#include "ChunkData.hpp"

struct LODChunkMeshTask {
  std::vector<ChunkVertex> vertices;
  std::vector<uint32_t> indices;
  glm::ivec2 pos;
  LODLevel lod_level;
};

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
  uint32_t mesh_handle{0};
  LODLevel lod_level{LODLevel::kNoMesh};

  enum class State { kNotFinished, kQueued, kFinished };
  State terrain_state{State::kNotFinished};
  State mesh_state{State::kNotFinished};

 private:
  glm::ivec3 pos_;
};
