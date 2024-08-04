#pragma once

#include "gameplay/world/Chunk.hpp"
#include "util/ThreadPool.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/vec3.hpp>
#include <unordered_map>

class BlockDB;
using ChunkMap = std::unordered_map<glm::ivec3, Chunk>;

class ChunkManager {
 public:
  explicit ChunkManager(BlockDB& block_db);
  ~ChunkManager();

  void Update(double dt);
  void Init();
  const ChunkMap& GetVisibleChunks() const { return chunk_map_; }
  void SetBlock(const glm::ivec3& pos, BlockType block);
  BlockType GetBlock(const glm::ivec3& pos) const;
  bool BlockPosExists(const glm::ivec3& world_pos) const;
  void OnImGui();

 private:
  ThreadPool thread_pool_;
  BlockDB& block_db_;
  ChunkMap chunk_map_;
  int load_distance_;
  std::vector<glm::ivec3> remesh_chunks_;

  uint32_t num_mesh_creations_{0};
  uint32_t total_vertex_count_{0};
  uint32_t total_index_count_{0};
};
