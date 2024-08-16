#include "ChunkManager.hpp"

#include <imgui.h>

#include "application/SettingsManager.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkData.hpp"
#include "gameplay/world/ChunkDef.hpp"
#include "gameplay/world/ChunkHelpers.hpp"
#include "gameplay/world/ChunkUtil.hpp"
#include "gameplay/world/TerrainGenerator.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Constants.hpp"
#include "renderer/Renderer.hpp"
#include "util/Timer.hpp"

namespace {

// dp::thread_pool thread_pool(std::thread::hardware_concurrency() - 2);

constexpr const int ChunkNeighborOffsets[27][3] = {
    {-1, -1, -1}, {-1, -1, 0}, {-1, -1, 1}, {-1, 0, -1}, {-1, 0, 0},  {-1, 0, 1}, {-1, 1, -1},
    {-1, 1, 0},   {-1, 1, 1},  {0, -1, -1}, {0, -1, 0},  {0, -1, 1},  {0, 0, -1}, {0, 0, 0},
    {0, 0, 1},    {0, 1, -1},  {0, 1, 0},   {0, 1, 1},   {1, -1, -1}, {1, -1, 0}, {1, -1, 1},
    {1, 0, -1},   {1, 0, 0},   {1, 0, 1},   {1, 1, -1},  {1, 1, 0},   {1, 1, 1},

};

}  // namespace

// TODO: find the best meshing memory pool size
ChunkManager::ChunkManager(BlockDB& block_db) : block_db_(block_db) {
  auto settings = SettingsManager::Get().LoadSetting("chunk_manager");
  load_distance_ = settings.value("load_distance", 16);
  frequency_ = settings.value("frequency", 1.0);
  if (load_distance_ >= 64) load_distance_ = 64;
  if (load_distance_ <= 0) load_distance_ = 1;
  terrain_.Load(block_db);
}

void ChunkManager::SetBlock(const glm::ivec3& pos, BlockType block) {
  auto chunk_pos = util::chunk::WorldToChunkPos(pos);
  auto it = chunk_map_.find(chunk_pos);
  EASSERT_MSG(it != chunk_map_.end(), "Set block in non existent chunk");
  glm::ivec3 block_pos_in_chunk = util::chunk::WorldToPosInChunk(pos);
  it->second->data.SetBlock(block_pos_in_chunk, block);
  AddRelatedChunks(block_pos_in_chunk, chunk_pos, chunk_mesh_queue_immediate_);
}

BlockType ChunkManager::GetBlock(const glm::ivec3& pos) const {
  auto it = chunk_map_.find(util::chunk::WorldToChunkPos(pos));
  EASSERT_MSG(it != chunk_map_.end(), "Get block in non existent chunk");
  return it->second->data.GetBlock(util::chunk::WorldToPosInChunk(pos));
}

Chunk* ChunkManager::GetChunk(const glm::ivec3& pos) {
  auto it = chunk_map_.find(util::chunk::WorldToChunkPos(pos));
  if (it == chunk_map_.end()) return nullptr;
  return it->second.get();
}

void ChunkManager::Update(double /*dt*/) {
  bool pos_changed = center_ != prev_center_;
  ZoneScoped;
  // TODO: implement better tick system
  static uint32_t tick_count = 0;
  tick_count++;
  if (tick_count == UINT32_MAX) {
    tick_count = 0;
  }
  static Timer timer;

  if (tick_count % 4 == 0) {
    ZoneScopedN("Process chunk terrain");
    while (!chunk_terrain_queue_.empty()) {
      auto pos = chunk_terrain_queue_.front();
      chunk_terrain_queue_.pop();
      // Chunk already exists at this point so no check
      thread_pool_.detach_task([this, pos] {
        ZoneScopedN("chunk terrain task");
        ChunkData data;
        TerrainGenerator gen{data, pos * ChunkLength, seed_, terrain_};
        // gen.GenerateNoise(3, frequency_);
        gen.GenerateBiome();
        {
          std::lock_guard<std::mutex> lock(mutex_);
          finished_chunk_terrain_queue_.emplace_back(pos, std::move(data));
        }
      });
    }
  }

  if (tick_count % 2 == 0) {
    ZoneScopedN("Process finsihed chunk terrain tasks");
    while (!finished_chunk_terrain_queue_.empty()) {
      auto& task = finished_chunk_terrain_queue_.front();
      auto it = chunk_map_.find(finished_chunk_terrain_queue_.front().first);
      if (it != chunk_map_.end()) {
        it->second->data = std::move(task.second);
        it->second->terrain_state = Chunk::State::Finished;
        state_stats_.loaded_chunks++;
      }
      finished_chunk_terrain_queue_.pop_front();
    }
  }

  state_stats_.max_meshed_chunks = ((load_distance_ - 1) * 2 + 1) * ((load_distance_ - 1) * 2 + 1);
  state_stats_.max_loaded_chunks = (load_distance_ * 2 + 1) * (load_distance_ * 2 + 1);
  // bool loaded = state_stats_.meshed_chunks >= state_stats_.max_meshed_chunks;

  // TODO: more robust ticks
  // if ((!loaded && tick_count % 2 == 0) || tick_count % 8 == 0) {
  {
    ZoneScopedN("Iterate chunks in range");
    // Clockwise Spiral iterator starting from center
    constexpr static int Dx[] = {1, 0, -1, 0};
    constexpr static int Dy[] = {0, 1, 0, -1};
    int direction = 0;
    int step_radius = 1;
    int direction_steps_counter = 0;
    int turn_counter = 0;
    int load_len = (load_distance_) * 2 + 1;
    int num_chunk_positions = load_len * load_len;
    // First index such that highly likely doesn't have outside neighbor chunks such that a mesh
    // can be created
    int first_non_meshable_chunk_pos_idx = num_chunk_positions - (load_len * 4 - 4);
    glm::ivec3 pos = center_;
    // TODO: infinite load height
    pos.y = 0;

    // Throttle the number of chunks sent to the queue per frame so that mesh tasks are interleaved
    int terrain_tasks_enqueued = 0;
    int max_terrain_enqueues_per_tick = SettingsManager::Get().CoreCount() * 4;
    // iterate through the chunks in range
    for (int chunk_pos_idx = 0; chunk_pos_idx < num_chunk_positions; chunk_pos_idx++) {
      auto it = chunk_map_.find(pos);
      // Make the chunk and enqueue for terrain if it doesn't exist yet
      if (it == chunk_map_.end()) {
        terrain_tasks_enqueued++;
        if (terrain_tasks_enqueued > max_terrain_enqueues_per_tick) break;
        for (pos.y = 0; pos.y <= NumVerticalChunks; pos.y++) {
          auto new_chunk = chunk_map_.emplace(pos, std::make_shared<Chunk>(pos));
          new_chunk.first->second->terrain_state = Chunk::State::Queued;
          chunk_terrain_queue_.emplace(pos);
        }
        // Add top and bottom chunks for padding
        // TODO: either allow infinite chunks vertically or cap it out and optimize
        glm::ivec3 pos_neg_y = {pos.x, -1, pos.z};
        glm::ivec3 pos_pos_y = {pos.x, NumVerticalChunks + 1, pos.z};
        {
          auto pos1 = chunk_map_.try_emplace(pos_pos_y, std::make_shared<Chunk>(pos_pos_y));
          auto pos2 = chunk_map_.try_emplace(pos_neg_y, std::make_shared<Chunk>(pos_neg_y));
          pos1.first->second->terrain_state = Chunk::State::Finished;
          pos2.first->second->terrain_state = Chunk::State::Finished;
        }
        // TODO: handle diff heights
      } else if (it->second->terrain_state == Chunk::State::NotFinished) {
        // This case should never happen since any new chunk should be queued or finished. Avoid in
        // debug mode
        EASSERT(0);
        chunk_terrain_queue_.emplace(pos);
      } else if (chunk_pos_idx < first_non_meshable_chunk_pos_idx) {
        for (pos.y = 0; pos.y <= NumVerticalChunks; pos.y++) {
          auto vert_it = chunk_map_.find(pos);
          EASSERT(vert_it != chunk_map_.end());
          // If the chunk is in meshing range, has terrain finished, and mesh hasn't been queued or
          // finished, and has blocks to mesh, and all its neighbors have finished terrain gen, send
          // to mesh queue
          if (vert_it->second->terrain_state == Chunk::State::Finished &&
              vert_it->second->mesh_state == Chunk::State::NotFinished &&
              vert_it->second->data.GetBlockCount() > 0) {
            bool all_neighbors_terrain_finished = true;
            for (const auto& off : ChunkNeighborOffsets) {
              auto neighbor_it = chunk_map_.find({pos.x + off[0], pos.y + off[1], pos.z + off[2]});
              if (neighbor_it == chunk_map_.end() ||
                  vert_it->second->terrain_state != Chunk::State::Finished) {
                // spdlog::info("{} {} {} {} {} {}", pos.x, pos.y, pos.z, pos.x + off[0],
                //              pos.y + off[1], pos.z + off[2]);
                all_neighbors_terrain_finished = false;
                break;
              }
            }
            if (all_neighbors_terrain_finished) {
              chunk_mesh_queue_.emplace(pos);
            }
          }
        }
      }

      // branchless iterate
      direction_steps_counter++;
      pos.x += Dx[direction];
      pos.z += Dy[direction];
      bool change_dir = direction_steps_counter == step_radius;
      direction = (direction + change_dir) % 4;
      direction_steps_counter *= !change_dir;
      turn_counter += change_dir;
      step_radius += change_dir * (1 - (turn_counter % 2));
    }
  }

  if ((tick_count + 3) % 2 == 0) {
    ZoneScopedN("Process mesh chunks");
    // process remesh chunks
    while (!chunk_mesh_queue_.empty()) {
      auto pos = chunk_mesh_queue_.front();
      chunk_mesh_queue_.pop();
      auto it = chunk_map_.find(pos);
      if (it == chunk_map_.end()) continue;
      Chunk& chunk = *it->second;
      if (chunk.data.GetBlockCount() == 0) continue;
      if (chunk.mesh_state == Chunk::State::Queued) continue;
      chunk.mesh_state = Chunk::State::Queued;
      if (chunk.mesh_handle != 0) Renderer::Get().FreeChunkMesh(chunk.mesh_handle);
      ChunkNeighborArray a;
      PopulateChunkNeighbors(a, pos, true);
      thread_pool_.detach_task([this, a, pos] {
        ChunkMesher mesher{block_db_.GetBlockData(), block_db_.GetMeshData()};
        std::vector<ChunkVertex> vertices;
        std::vector<uint32_t> indices;
        if (mesh_greedy_) {
          // mesher.GenerateLODGreedy(a[13]->data, vertices, indices);
          mesher.GenerateGreedy(a, vertices, indices);
        } else {
          mesher.GenerateSmart(a, vertices, indices);
        }
        {
          std::lock_guard<std::mutex> lock(mutex_);
          chunk_mesh_finished_queue_.emplace_back(vertices, indices, pos);
        }
      });
    }
  }

  if ((tick_count + 2) % 2 == 0) {
    ZoneScopedN("Process finished mesh chunks");
    Timer timer;
    while (!chunk_mesh_finished_queue_.empty()) {
      if (timer.ElapsedMS() > 5) break;
      auto& task = chunk_mesh_finished_queue_.front();
      auto it = chunk_map_.find(task.pos);
      if (it != chunk_map_.end()) {
        if (!task.vertices.empty()) {
          it->second->mesh_handle = Renderer::Get().AllocateStaticChunk(task.vertices, task.indices,
                                                                        task.pos * ChunkLength);
        }
        it->second->mesh_state = Chunk::State::Finished;
        state_stats_.meshed_chunks++;
      }
      chunk_mesh_finished_queue_.pop_front();
    }
  }

  {
    ZoneScopedN("Immediate chunk remesh");
    for (const auto& pos : chunk_mesh_queue_immediate_) {
      auto it = chunk_map_.find(pos);
      if (it == chunk_map_.end()) continue;
      auto& chunk = it->second;
      bool can_mesh = true;
      for (const auto& offset : ChunkNeighborOffsets) {
        if (!chunk_map_.contains(pos + glm::ivec3{offset[0], offset[1], offset[2]})) {
          can_mesh = false;
          break;
        }
      }
      if (!can_mesh) continue;

      ChunkNeighborArray a;
      PopulateChunkNeighbors(a, pos, true);
      ChunkMesher mesher{block_db_.GetBlockData(), block_db_.GetMeshData()};
      std::vector<ChunkVertex> vertices;
      std::vector<uint32_t> indices;
      if (mesh_greedy_)
        mesher.GenerateGreedy(a, vertices, indices);
      else
        mesher.GenerateSmart(a, vertices, indices);
      if (chunk->mesh_state == Chunk::State::Finished)
        Renderer::Get().FreeStaticChunkMesh(chunk->mesh_handle);
      chunk->mesh_handle =
          Renderer::Get().AllocateStaticChunk(vertices, indices, pos * ChunkLength);
      chunk->mesh_state = Chunk::State::Finished;
    }
    chunk_mesh_queue_immediate_.clear();
  }

  if (pos_changed) UnloadChunksOutOfRange();
}
void ChunkManager::UnloadChunksOutOfRange() {
  ZoneScoped;
  for (auto it = chunk_map_.begin(); it != chunk_map_.end();) {
    if (abs(it->first.x - center_.x) > load_distance_ ||
        abs(it->first.z - center_.z) > load_distance_) {
      if (it->second->mesh_handle != 0)
        Renderer::Get().FreeStaticChunkMesh(it->second->mesh_handle);
      it = chunk_map_.erase(it);
      state_stats_.meshed_chunks--;
      state_stats_.loaded_chunks--;
    } else {
      it++;
    }
  }
}

void ChunkManager::PopulateChunkStatePixels(std::vector<uint8_t>& pixels, glm::ivec2& out_dims,
                                            int y_level, float opacity) {
  ZoneScoped;
  int length = (load_distance_) * 2 + 1;
  out_dims.x = length;
  out_dims.y = length;
  glm::ivec3 iter;
  iter.y = y_level;
  pixels.clear();
  pixels.reserve(length * length * 4);
  auto alpha = static_cast<uint8_t>(opacity * 255);
  auto add_color = [&pixels, &alpha](const glm::ivec3& col) {
    pixels.emplace_back(col.r);
    pixels.emplace_back(col.g);
    pixels.emplace_back(col.b);
    pixels.emplace_back(alpha);
  };
  for (iter.z = center_.z - load_distance_; iter.z <= center_.z + load_distance_; iter.z++) {
    for (iter.x = center_.x - load_distance_; iter.x <= center_.x + load_distance_; iter.x++) {
      if (iter.x == center_.x && iter.z == center_.z) {
        add_color(color::White);
      } else {
        auto it = chunk_map_.find(iter);
        if (it == chunk_map_.end()) {
          add_color(color::Black);
        } else {
          Chunk& chunk = *it->second;
          if (chunk.mesh_handle != 0) {
            add_color(color::Blue);
          } else if (chunk.mesh_state == Chunk::State::Finished) {
            add_color(color::Green);
          } else if (chunk.terrain_state == Chunk::State::Finished) {
            add_color(color::Red);
          } else {
            add_color(color::Cyan);
          }
        }
      }
    }
  }
}

ChunkManager::~ChunkManager() {
  thread_pool_.wait();
  for (auto& [pos, chunk] : chunk_map_) {
    Renderer::Get().FreeStaticChunkMesh(chunk->mesh_handle);
  }
  nlohmann::json j = {{"load_distance", load_distance_}, {"frequency", frequency_}};
  SettingsManager::Get().SaveSetting(j, "chunk_manager");
}

void ChunkManager::OnImGui() {
  if (ImGui::CollapsingHeader("Chunk Manager", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::SliderInt("Load Distance", &load_distance_, 1, 72)) {
      UnloadChunksOutOfRange();
    }
    ImGui::Checkbox("Greedy Meshing", &mesh_greedy_);
    ImGui::SliderFloat("Frequency", &frequency_, 0.1, 10);
    if (ImGui::CollapsingHeader("Stats##chunk_manager_stats", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("Center: %i %i %i", center_.x, center_.y, center_.z);
      ImGui::Text("Prev Center: %i %i %i", prev_center_.x, prev_center_.y, prev_center_.z);
      ImGui::Text("Chunk Map: %zu", chunk_map_.size());
      ImGui::Text("Chunk Mesh Queue:  %zu", chunk_mesh_queue_.size());
      ImGui::Text("Chunk Mesh Queue Imm:  %zu", chunk_mesh_queue_immediate_.size());
      ImGui::Text("Chunk Mesh Finish Queue:  %zu", chunk_mesh_finished_queue_.size());
      ImGui::Text("Chunk Terrain Queue:  %zu", chunk_terrain_queue_.size());
      ImGui::Text("Chunk Terrain Finish Queue:  %zu", finished_chunk_terrain_queue_.size());
      ImGui::Text("Chunks Loaded: %i, Max: %i", state_stats_.loaded_chunks,
                  state_stats_.max_loaded_chunks);
      ImGui::Text("Chunks Meshed: %i, Max: %i", state_stats_.meshed_chunks,
                  state_stats_.max_meshed_chunks);
    }
  }
}

bool ChunkManager::BlockPosExists(const glm::ivec3& world_pos) const {
  return chunk_map_.contains(util::chunk::WorldToChunkPos(world_pos));
}

void ChunkManager::PopulateChunkNeighbors(ChunkNeighborArray& neighbor_array, const glm::ivec3& pos,
                                          bool add_new_chunks) {
  ZoneScoped;
  for (const auto& off : ChunkNeighborOffsets) {
    glm::ivec3 neighbor_pos = {pos.x + off[0], pos.y + off[1], pos.z + off[2]};
    auto neighbor_it = chunk_map_.find(neighbor_pos);
    if (neighbor_it == chunk_map_.end()) {
      if (add_new_chunks) {
        auto new_chunk =
            chunk_map_.try_emplace(neighbor_pos, std::make_shared<Chunk>(neighbor_pos));
        neighbor_array[ChunkNeighborOffsetToIdx(off[0], off[1], off[2])] = new_chunk.first->second;
      } else {
        neighbor_array[ChunkNeighborOffsetToIdx(off[0], off[1], off[2])] = nullptr;
      }
    } else {
      neighbor_array[ChunkNeighborOffsetToIdx(off[0], off[1], off[2])] = neighbor_it->second;
    }
  }
}

void ChunkManager::AddRelatedChunks(const glm::ivec3& block_pos_in_chunk,
                                    const glm::ivec3& chunk_pos,
                                    std::unordered_set<glm::ivec3>& chunk_set) {
  ZoneScoped;
  glm::ivec3 chunks_to_add[8];  // at most 8 chunks are related to a block
  glm::ivec3 temp;  // temp variable to store the chunk to add (calculate offset from chunk pos)
  int num_chunks_to_add = 1;  // always add the chunk the block is in
  int size;
  chunks_to_add[0] = chunk_pos;  // always add the chunk the block is in

  // iterate over each axis
  for (int axis = 0; axis < 3; axis++) {
    // if block is on edge of the axis, add the chunks on the other side of the edge
    if (block_pos_in_chunk[axis] == 0) {
      size = num_chunks_to_add;
      for (int i = 0; i < size; i++) {
        temp = chunks_to_add[i];  // works since only doing one axis at a time
        temp[axis]--;             // decrement chunk pos on the axis
        chunks_to_add[num_chunks_to_add++] = temp;
      }
    }

    if (block_pos_in_chunk[axis] == ChunkLength - 1) {
      size = num_chunks_to_add;
      for (int i = 0; i < size; i++) {
        temp = chunks_to_add[i];
        temp[axis]++;
        chunks_to_add[num_chunks_to_add++] = temp;
      }
    }
  }

  // add the chunks to the set
  for (int i = 0; i < num_chunks_to_add; i++) {
    chunk_set.insert(chunks_to_add[i]);
  }
}

void ChunkManager::SetCenter(const glm::vec3& world_pos) {
  prev_center_ = center_;
  center_ = util::chunk::WorldToChunkPos(world_pos);
}

void ChunkManager::SetSeed(int seed) { seed_ = seed; }
