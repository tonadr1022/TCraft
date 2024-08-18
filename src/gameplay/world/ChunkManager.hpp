#pragma once

#include <BS_thread_pool.hpp>
#include <deque>
#include <libcuckoo/cuckoohash_map.hh>

#include "gameplay/world/Chunk.hpp"
#include "gameplay/world/Terrain.hpp"

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/hash.hpp>
#include <glm/vec3.hpp>

#include "gameplay/world/ChunkData.hpp"

class BlockDB;

using ChunkMap = libcuckoo::cuckoohash_map<glm::ivec3, std::shared_ptr<Chunk>>;
// using ChunkMap = std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>>;

struct ChunkStateData {
  std::vector<ChunkState> data;
  int width;
  int height;
};

struct LODChunkMeshTask {
  std::vector<ChunkVertex> vertices;
  std::vector<uint32_t> indices;
  glm::ivec2 pos;
  LODLevel lod_level;
};

class ChunkManager {
 public:
  explicit ChunkManager(BlockDB& block_db);
  ~ChunkManager();

  void AddNewChunks(bool first_load, bool throttle = false);
  void Init(const glm::ivec3& start_pos);
  void Update(double dt);
  // TODO: either make non trivial or remove
  void SetSeed(int seed);
  const ChunkMap& GetVisibleChunks() const { return chunk_map_; }
  void SetBlock(const glm::ivec3& pos, BlockType block);
  BlockType GetBlock(const glm::ivec3& pos) const;
  Chunk* GetChunk(const glm::ivec3& pos);
  bool BlockPosExists(const glm::ivec3& world_pos) const;
  void OnImGui();
  void SetCenter(const glm::vec3& world_pos);

  enum class ChunkMapMode { ChunkState, LODLevels, Count };
  void PopulateChunkStatePixels(std::vector<uint8_t>& pixels, glm::ivec2& out_dims, int y_level,
                                float opacity, ChunkMapMode mode);
  void UnloadChunksOutOfRange(const glm::ivec3& diff);
  void UnloadChunksOutOfRange(int old_load_distance);

  struct StateStats {
    uint32_t max_chunks{};
    uint32_t loaded_chunks{};
    uint32_t meshed_chunks{};
  };

  const StateStats& GetStateStats() const { return state_stats_; }

 private:
  BlockDB& block_db_;
  ChunkMap chunk_map_;
  Terrain terrain_;
  int seed_{};
  int load_distance_{};
  glm::ivec3 center_{};
  glm::ivec3 prev_center_{};
  std::mutex mutex_;
  std::queue<glm::ivec2> chunk_mesh_queue_;
  std::unordered_set<glm::ivec3> chunk_mesh_queue_immediate_;
  std::deque<ChunkMeshTask> chunk_mesh_finished_queue_;
  using PositionIteratorFunc = std::function<void(const glm::ivec2&)>;
  using PositionIteratorFuncIdx = std::function<void(const glm::ivec2&, int)>;
  using VerticalPositionIteratorFunc = std::function<void(const glm::ivec3&)>;
  void IterateChunks(int load_distance, const PositionIteratorFunc& func) const;
  void IterateChunks(int load_distance, const PositionIteratorFuncIdx& func) const;
  void IterateChunks(int start_distance, int load_distance, const PositionIteratorFunc& func) const;
  void IterateChunks(int start_distance, int load_distance,
                     const PositionIteratorFuncIdx& func) const;
  void IterateChunksVertical(int load_distance, const VerticalPositionIteratorFunc& func) const;

  std::queue<glm::ivec2> chunk_terrain_queue_;
  std::deque<glm::ivec2> finished_chunk_terrain_queue_;

  void PopulateChunkNeighbors(ChunkNeighborArray& neighbor_array, const glm::ivec3& pos,
                              bool add_new_chunks);
  static void AddRelatedChunks(const glm::ivec3& block_pos_in_chunk, const glm::ivec3& chunk_pos,
                               std::unordered_set<glm::ivec3>& chunk_set);
  bool ChunkPosOutsideHorizontalRange(int x, int z, int dist, const glm::ivec2& pos);
  bool mesh_greedy_{true};

  float frequency_;
  StateStats state_stats_;
  int chunk_dist_lod_1_{5};
  bool first_load_completed_{false};

  BS::thread_pool thread_pool_;

  void FreeChunkMesh(uint32_t& handle);
  void SendChunkMeshTaskNoLOD(const glm::ivec3& pos);
  void SendChunkMeshTaskLOD1(const glm::ivec2& pos);
  void AllocateChunkMesh();
};
