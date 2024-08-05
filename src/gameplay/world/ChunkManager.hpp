#pragma once

#include <queue>

#include "gameplay/world/Chunk.hpp"

#define GLM_ENABLE_EXPERIMENTAL

#include <thread_pool/thread_pool.h>

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
  BlockDB& block_db_;
  ChunkMap chunk_map_;
  int load_distance_;
  std::mutex queue_mutex_;
  std::vector<glm::ivec3> remesh_chunk_tasks_positions_;
  std::vector<glm::ivec3> immediate_remesh_chunk_task_positions_;
  std::queue<ChunkMeshTask> finished_chunk_mesh_tasks_;

  std::vector<glm::ivec3> chunk_terrain_task_positions_;
  std::queue<std::pair<glm::ivec3, ChunkData>> finished_chunk_terrain_tasks_;

  std::vector<glm::ivec3> meshing_candidate_positions_;

  uint32_t num_mesh_creations_{0};
  uint32_t total_vertex_count_{0};
  uint32_t total_index_count_{0};
};
