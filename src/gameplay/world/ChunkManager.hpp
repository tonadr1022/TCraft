#pragma once

#include <deque>

#include "gameplay/world/Chunk.hpp"
#include "util/MemoryPool.hpp"

#define GLM_ENABLE_EXPERIMENTAL

#include <thread_pool/thread_pool.h>

#include <glm/gtx/hash.hpp>
#include <glm/vec3.hpp>
#include <unordered_map>

#include "gameplay/world/ChunkData.hpp"

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
  Chunk* GetChunk(const glm::ivec3& pos);
  bool BlockPosExists(const glm::ivec3& world_pos) const;
  void OnImGui();
  void SetCenter(const glm::vec3& world_pos);

 private:
  BlockDB& block_db_;
  ChunkMap chunk_map_;
  int load_distance_;
  glm::ivec3 center_;
  glm::ivec3 prev_center_;
  std::mutex mutex_;
  std::vector<glm::ivec3> chunk_mesh_queue_;
  std::unordered_set<glm::ivec3> chunk_mesh_queue_immediate_;
  std::deque<ChunkMeshTask> chunk_mesh_finished_queue_;

  std::vector<glm::ivec3> chunk_terrain_queue_;
  std::deque<std::pair<glm::ivec3, ChunkData>> finished_chunk_terrain_queue_;

  uint32_t num_mesh_creations_{0};
  uint32_t total_vertex_count_{0};
  uint32_t total_index_count_{0};

  void PopulateChunkNeighbors(ChunkNeighborArray& neighbor_array, const glm::ivec3& pos,
                              bool add_new_chunks);
  static void AddRelatedChunks(const glm::ivec3& block_pos_in_chunk, const glm::ivec3& chunk_pos,
                               std::unordered_set<glm::ivec3>& chunk_set);
  // void PopulateChunkNeighborDataArray(ChunkData* chunk_data[27],
  //                                     std::vector<BlockType>& block_data);
  bool mesh_greedy_{true};
};
