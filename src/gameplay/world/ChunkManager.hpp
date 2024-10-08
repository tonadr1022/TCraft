#pragma once

#include <BS_thread_pool.hpp>
#include <deque>
#include <libcuckoo/cuckoohash_map.hh>

#include "gameplay/world/Chunk.hpp"
#include "gameplay/world/Terrain.hpp"

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/hash.hpp>
#include <glm/vec3.hpp>

class BlockDB;

using ChunkMap = libcuckoo::cuckoohash_map<glm::ivec3, std::shared_ptr<Chunk>>;
using LODChunkMap = libcuckoo::cuckoohash_map<glm::ivec2, uint32_t>;
// using ChunkMap = std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>>;

struct ChunkStateData {
  std::vector<ChunkState> data;
  int width;
  int height;
};

class ChunkManager {
 public:
  explicit ChunkManager(BlockDB& block_db);
  ~ChunkManager();

  void AddNewChunks(bool throttle);
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
  bool IsLoaded() const;

 private:
  BlockDB& block_db_;
  ChunkMap chunk_map_;
  LODChunkMap lod_chunk_handle_map_;
  Terrain terrain_;
  int seed_{};
  int load_distance_{};
  glm::ivec3 center_{};
  glm::ivec3 prev_center_{};
  std::queue<glm::ivec2> chunk_mesh_queue_;
  std::unordered_set<glm::ivec3> chunk_mesh_queue_immediate_;
  std::mutex lod_chunk_mesh_finish_mtx_;
  std::mutex chunk_mesh_finish_mtx_;
  std::queue<ChunkMeshTask> chunk_mesh_finished_queue_;
  std::queue<LODChunkMeshTask> lod_chunk_mesh_finished_queue_;
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
  std::mutex chunk_terrain_finish_mtx_;
  std::deque<glm::ivec2> finished_chunk_terrain_queue_;

  void PopulateChunkNeighbors(ChunkNeighborArray& neighbor_array, const glm::ivec3& pos);
  static void AddRelatedChunks(const glm::ivec3& block_pos_in_chunk, const glm::ivec3& chunk_pos,
                               std::unordered_set<glm::ivec3>& chunk_set);
  bool ChunkPosOutsideHorizontalRange(int x, int z, int dist, const glm::ivec2& pos);
  bool ChunkPosWithinDistance(int x, int z, int distance) const;

  float frequency_;
  StateStats state_stats_;
  int lod_1_load_distance_{5};
  bool first_load_completed_{false};
  bool update_chunks_on_move_{true};

  BS::thread_pool thread_pool_;

  void FreeChunkMesh(ChunkMesh& mesh);
  void FreeOpaqueChunkMesh(uint32_t& handle);
  void SendChunkMeshTaskNoLOD(const glm::ivec3& pos);
  void SendChunkMeshTaskLOD1(const glm::ivec2& pos);
  void AllocateChunkMesh();
};
