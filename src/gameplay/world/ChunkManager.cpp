#include "ChunkManager.hpp"

#include <imgui.h>

#include "application/SettingsManager.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/Chunk.hpp"
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
  EASSERT_MSG(chunk_map_.contains(chunk_pos), "Set block in non existent chunk");
  chunk_map_.find_fn(chunk_pos,
                     [this, &chunk_pos, &pos, block](const std::shared_ptr<Chunk>& chunk) {
                       glm::ivec3 block_pos_in_chunk = util::chunk::WorldToPosInChunk(pos);
                       chunk->data.SetBlock(block_pos_in_chunk, block);
                       AddRelatedChunks(block_pos_in_chunk, chunk_pos, chunk_mesh_queue_immediate_);
                     });
}

BlockType ChunkManager::GetBlock(const glm::ivec3& pos) const {
  // debug only
  EASSERT_MSG(chunk_map_.contains(util::chunk::WorldToChunkPos(pos)),
              "Get block in non existent chunk");
  BlockType block{0};
  chunk_map_.find_fn(util::chunk::WorldToChunkPos(pos),
                     [&block, &pos](const std::shared_ptr<Chunk>& chunk) {
                       block = chunk->data.GetBlock(util::chunk::WorldToPosInChunk(pos));
                     });
  return block;
}

Chunk* ChunkManager::GetChunk(const glm::ivec3& pos) {
  Chunk* ret = nullptr;
  chunk_map_.find_fn(util::chunk::WorldToChunkPos(pos),
                     [&ret](const std::shared_ptr<Chunk>& chunk) { ret = chunk.get(); });
  return ret;
}

void ChunkManager::Update(double /*dt*/) {
  bool pos_changed = center_ != prev_center_;
  ZoneScoped;
  // TODO: implement better tick system
  static uint32_t tick_count = 0;
  tick_count = (tick_count + 1) % UINT32_MAX;

  static Timer timer;

  {
    ZoneScopedN("Process chunk terrain");
    while (!chunk_terrain_queue_.empty()) {
      auto pos = chunk_terrain_queue_.front();
      chunk_terrain_queue_.pop();
      // Chunk already exists at this point so no check
      std::array<std::shared_ptr<Chunk>, NumVerticalChunks> data;
      glm::ivec3 p;
      p.x = pos.x;
      p.z = pos.y;
      bool valid = true;
      for (p.y = 0; p.y < NumVerticalChunks; p.y++) {
        if (!chunk_map_.find_fn(
                p, [&p, &data](const std::shared_ptr<Chunk>& chunk) { data[p.y] = chunk; })) {
          spdlog::error("uh oh no chunk, {} {} {}", pos.x, p.y, pos.y);
          valid = false;
          break;
        }
      }
      if (!valid) continue;
      thread_pool_.detach_task([this, data, pos] {
        ZoneScopedN("chunk terrain task");

        TerrainGenerator gen{data, pos * ChunkLength, seed_, terrain_};
        // gen.GenerateSolid(3);
        gen.GenerateBiome();
        for (int i = 0; i < NumVerticalChunks; i++) {
          data[i]->data.DownSample();
          // TODO: see if race condition somewhere?
          data[i]->terrain_state = Chunk::State::Finished;
        }
        {
          std::lock_guard<std::mutex> lock(mutex_);
          // for (int i = 0; i < NumVerticalChunks; i++) {
          // }
          // state_stats_.loaded_chunks += NumVerticalChunks;
          // chunk_mesh_queue_.emplace(pos);
          finished_chunk_terrain_queue_.emplace_back(pos);
        }
      });
    }
  }

  {
    ZoneScopedN("Process finished chunk terrain tasks");
    while (!finished_chunk_terrain_queue_.empty()) {
      auto pos = finished_chunk_terrain_queue_.front();
      glm::ivec3 p;
      p.x = pos.x;
      p.z = pos.y;
      for (p.y = 0; p.y < NumVerticalChunks; p.y++) {
        chunk_map_.find_fn(p, [this](const std::shared_ptr<Chunk>& chunk) {
          chunk->terrain_state = Chunk::State::Finished;
          state_stats_.loaded_chunks++;
        });
      }
      finished_chunk_terrain_queue_.pop_front();
      // spdlog::info("{} {}", pos.x, pos.y);
      chunk_mesh_queue_.emplace(pos);
    }
  }

  state_stats_.max_chunks = (load_distance_ * 2 + 1) * (load_distance_ * 2 + 1) * NumVerticalChunks;

  {
    ZoneScopedN("Process mesh chunks");
    // process remesh chunks
    while (!chunk_mesh_queue_.empty()) {
      glm::ivec2 pos = chunk_mesh_queue_.front();
      glm::ivec3 p;
      p.x = pos.x;
      p.z = pos.y;
      chunk_mesh_queue_.pop();
      // TODO: make queued state before the queue itself
      bool valid = true;
      for (p.y = 0; p.y < NumVerticalChunks; p.y++) {
        if (!chunk_map_.find_fn(p, [](const std::shared_ptr<Chunk>& chunk) {
              chunk->mesh_state = Chunk::State::Queued;
            })) {
          valid = false;
          break;
        }
      }
      if (!valid) continue;

      if (p.x < center_.x - chunk_dist_lod_1_ || p.x > center_.x + chunk_dist_lod_1_ ||
          p.z < center_.z - chunk_dist_lod_1_ || p.z > center_.z + chunk_dist_lod_1_) {
        // spdlog::info("{} {}", pos.x, pos.y);
        SendChunkMeshTaskLOD1(pos);
      } else {
        for (p.y = 0; p.y < NumVerticalChunks; p.y++) {
          SendChunkMeshTaskNoLOD(p);
        }
      }
    }
  }

  if (state_stats_.loaded_chunks >= state_stats_.max_chunks) first_load_completed_ = true;
  // TODO: handle multiple direction change in one frame, ie teleportation
  if (pos_changed && first_load_completed_ && update_chunks_on_move_) {
    ZoneScopedN("Pos Changed");
    glm::ivec3 diff = center_ - prev_center_;
    if (diff.z == 1) {
      glm::ivec3 pos;
      for (pos.x = center_.x - chunk_dist_lod_1_; pos.x <= center_.x + chunk_dist_lod_1_; pos.x++) {
        pos.z = center_.z + chunk_dist_lod_1_;
        for (pos.y = 0; pos.y < NumVerticalChunks; pos.y++) {
          SendChunkMeshTaskNoLOD(pos);
        }
        SendChunkMeshTaskLOD1({pos.x, prev_center_.z - chunk_dist_lod_1_});
      }
    } else if (diff.z == -1) {
      glm::ivec3 pos;
      for (pos.x = center_.x - chunk_dist_lod_1_; pos.x <= center_.x + chunk_dist_lod_1_; pos.x++) {
        pos.z = center_.z - chunk_dist_lod_1_;
        for (pos.y = 0; pos.y < NumVerticalChunks; pos.y++) {
          SendChunkMeshTaskNoLOD(pos);
        }
        SendChunkMeshTaskLOD1({pos.x, prev_center_.z + chunk_dist_lod_1_});
      }
    } else if (diff.x == 1) {
      glm::ivec3 pos;
      for (pos.z = center_.z - chunk_dist_lod_1_; pos.z <= center_.z + chunk_dist_lod_1_; pos.z++) {
        pos.x = center_.x + chunk_dist_lod_1_;
        for (pos.y = 0; pos.y < NumVerticalChunks; pos.y++) {
          SendChunkMeshTaskNoLOD(pos);
        }
        SendChunkMeshTaskLOD1({prev_center_.x - chunk_dist_lod_1_, pos.z});
      }
    } else if (diff.x == -1) {
      glm::ivec3 pos;
      for (pos.z = center_.z - chunk_dist_lod_1_; pos.z <= center_.z + chunk_dist_lod_1_; pos.z++) {
        pos.x = center_.x - chunk_dist_lod_1_;
        for (pos.y = 0; pos.y < NumVerticalChunks; pos.y++) {
          SendChunkMeshTaskNoLOD(pos);
        }
        SendChunkMeshTaskLOD1({prev_center_.x + chunk_dist_lod_1_, pos.z});
      }
    }
  }

  {
    ZoneScopedN("Process finished mesh chunks");
    static Timer timer;
    timer.Reset();
    while (!chunk_mesh_finished_queue_.empty()) {
      if (timer.ElapsedMS() > 5) break;
      auto& task = chunk_mesh_finished_queue_.front();
      chunk_map_.find_fn(task.pos, [this, &task](const std::shared_ptr<Chunk>& chunk) {
        FreeChunkMesh(chunk->mesh_handle);
        lod_chunk_map_.find_fn(glm::ivec2{task.pos.x, task.pos.z},
                               [this](uint32_t& handle) { FreeChunkMesh(handle); });

        if (!task.vertices.empty()) {
          chunk->mesh_handle = Renderer::Get().AllocateStaticChunk(
              task.vertices, task.indices, task.pos * ChunkLength, task.lod_level);
        }
        // TODO: get rid of lod level here since it's in a diff queue now
        chunk->lod_level = task.lod_level;
        chunk->mesh_state = Chunk::State::Finished;
        state_stats_.meshed_chunks++;
      });
      chunk_mesh_finished_queue_.pop();
    }

    while (!lod_chunk_mesh_finished_queue_.empty()) {
      auto& task = lod_chunk_mesh_finished_queue_.front();
      if (!lod_chunk_map_.find_fn(task.pos, [this, &task](uint32_t& handle) {
            FreeChunkMesh(handle);
            handle = Renderer::Get().AllocateStaticChunk(
                task.vertices, task.indices,
                glm::ivec3{task.pos.x * ChunkLength, 0, task.pos.y * ChunkLength}, task.lod_level);
          })) {
        lod_chunk_map_.insert(
            task.pos,
            Renderer::Get().AllocateStaticChunk(
                task.vertices, task.indices,
                glm::ivec3{task.pos.x * ChunkLength, 0, task.pos.y * ChunkLength}, task.lod_level));
      }
      lod_chunk_mesh_finished_queue_.pop();
    }
  }

  {
    ZoneScopedN("Immediate chunk remesh");
    for (const auto& pos : chunk_mesh_queue_immediate_) {
      if (!chunk_map_.contains(pos)) continue;
      auto chunk = chunk_map_.find(pos);
      if (chunk->data.GetBlockCount() == 0) continue;

      ChunkNeighborArray a;
      PopulateChunkNeighbors(a, pos, true);
      ChunkMesher mesher{block_db_.GetBlockData(), block_db_.GetMeshData()};
      std::vector<ChunkVertex> vertices;
      std::vector<uint32_t> indices;
      if (mesh_greedy_)
        mesher.GenerateGreedy(a, vertices, indices);
      else
        mesher.GenerateSmart(a, vertices, indices);

      FreeChunkMesh(chunk->mesh_handle);
      chunk->mesh_handle = Renderer::Get().AllocateStaticChunk(vertices, indices, pos * ChunkLength,
                                                               LODLevel::Regular);
      state_stats_.meshed_chunks++;
      chunk->mesh_state = Chunk::State::Finished;
    }
    chunk_mesh_queue_immediate_.clear();
  }

  if (!first_load_completed_) {
    AddNewChunks(false, true);
  }
  if (pos_changed && update_chunks_on_move_) {
    UnloadChunksOutOfRange(center_ - prev_center_);
    AddNewChunks(false, true);
  }
}

void ChunkManager::UnloadChunksOutOfRange(int old_load_distance) {
  ZoneScoped;
  IterateChunks(old_load_distance, [this](const glm::ivec2& pos) {
    if (abs(pos.x - center_.x) > load_distance_ || abs(pos.y - center_.z) > load_distance_) {
      glm::ivec3 p;
      p.x = pos.x;
      p.z = pos.y;
      for (p.y = 0; p.y < NumVerticalChunks; p.y++) {
        if (chunk_map_.find_fn(
                p, [this](const std::shared_ptr<Chunk>& c) { FreeChunkMesh(c->mesh_handle); })) {
          chunk_map_.erase(p);
        }
      }
    }
  });
}

void ChunkManager::UnloadChunksOutOfRange(const glm::ivec3& diff) {
  ZoneScoped;
  auto erase_fn = [this](const glm::ivec3& pos) {
    if (chunk_map_.find_fn(
            pos, [this](const std::shared_ptr<Chunk>& c) { FreeChunkMesh(c->mesh_handle); })) {
      chunk_map_.erase(pos);
    }
  };
  if (diff.z == 1) {
    glm::ivec3 pos;
    pos.z = prev_center_.z - load_distance_;
    for (pos.x = prev_center_.x - load_distance_; pos.x <= prev_center_.x + load_distance_;
         pos.x++) {
      for (pos.y = 0; pos.y < NumVerticalChunks; pos.y++) {
        erase_fn(pos);
      }
    }
  } else if (diff.z == -1) {
    glm::ivec3 pos;
    pos.z = prev_center_.z + load_distance_;
    for (pos.x = prev_center_.x - load_distance_; pos.x <= prev_center_.x + load_distance_;
         pos.x++) {
      for (pos.y = 0; pos.y < NumVerticalChunks; pos.y++) {
        erase_fn(pos);
      }
    }
  }
  if (diff.x == 1) {
    glm::ivec3 pos;
    pos.x = prev_center_.x - load_distance_;
    for (pos.z = prev_center_.z - load_distance_; pos.z <= prev_center_.z + load_distance_;
         pos.z++) {
      for (pos.y = 0; pos.y < NumVerticalChunks; pos.y++) {
        erase_fn(pos);
      }
    }
  } else if (diff.x == -1) {
    glm::ivec3 pos;
    pos.x = prev_center_.x + load_distance_;
    for (pos.z = prev_center_.z - load_distance_; pos.z <= prev_center_.z + load_distance_;
         pos.z++) {
      for (pos.y = 0; pos.y < NumVerticalChunks; pos.y++) {
        erase_fn(pos);
      }
    }
  }
}

void ChunkManager::PopulateChunkStatePixels(std::vector<uint8_t>& pixels, glm::ivec2& out_dims,
                                            int y_level, float opacity, ChunkMapMode mode) {
  ZoneScoped;
  int dist = load_distance_ + 1;
  int length = dist * 2 + 1;
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

  if (mode == ChunkMapMode::ChunkState) {
    for (iter.z = center_.z - dist; iter.z <= center_.z + dist; iter.z++) {
      for (iter.x = center_.x - dist; iter.x <= center_.x + dist; iter.x++) {
        if (iter.x == center_.x && iter.z == center_.z) {
          add_color(color::White);
        } else {
          if (!chunk_map_.find_fn(iter, [&add_color](const std::shared_ptr<Chunk>& chunk) {
                if (chunk->mesh_handle != 0) {
                  EASSERT(chunk->mesh_state == Chunk::State::Finished);
                  add_color(color::Blue);
                } else if (chunk->mesh_state == Chunk::State::Finished) {
                  add_color(color::Green);
                } else if (chunk->terrain_state == Chunk::State::Finished) {
                  add_color(color::Red);
                } else {
                  add_color(color::Cyan);
                }
              })) {
            add_color(color::Black);
          }
        }
      }
    }
  } else {
    for (iter.z = center_.z - dist; iter.z <= center_.z + dist; iter.z++) {
      for (iter.x = center_.x - dist; iter.x <= center_.x + dist; iter.x++) {
        if (iter.x == center_.x && iter.z == center_.z) {
          add_color(color::White);
        } else {
          if (!chunk_map_.find_fn(iter, [&add_color](const std::shared_ptr<Chunk>& chunk) {
                if (chunk->lod_level == LODLevel::NoMesh) {
                  add_color(color::Blue);
                } else if (chunk->lod_level == LODLevel::Regular) {
                  add_color(color::Green);
                } else if (chunk->lod_level == LODLevel::One) {
                  add_color(color::Red);
                } else {
                  add_color(color::Cyan);
                }
              })) {
            add_color(color::Black);
          }
        }
      }
    }
  }
}

void ChunkManager::IterateChunks(int start_distance, int load_distance,
                                 const PositionIteratorFunc& func) const {
  int start_idx = (start_distance * 2 + 1) * (start_distance * 2 + 1);
  IterateChunks(load_distance, [&func, start_idx](const glm::ivec2& pos, int i) {
    if (i >= start_idx) {
      func(pos);
    }
  });
}

void ChunkManager::IterateChunks(int load_distance, const PositionIteratorFuncIdx& func) const {
  // Clockwise Spiral iterator starting from center
  constexpr static int Dx[] = {1, 0, -1, 0};
  constexpr static int Dy[] = {0, 1, 0, -1};
  int direction = 0;
  int step_radius = 1;
  int direction_steps_counter = 0;
  int turn_counter = 0;
  int load_len = (load_distance) * 2 + 1;
  int num_chunk_positions = load_len * load_len;
  glm::ivec2 pos{center_.x, center_.z};

  for (int chunk_pos_idx = 0; chunk_pos_idx < num_chunk_positions; chunk_pos_idx++) {
    func(pos, chunk_pos_idx);
    // branchless iterate
    direction_steps_counter++;
    pos.x += Dx[direction];
    pos.y += Dy[direction];
    bool change_dir = direction_steps_counter == step_radius;
    direction = (direction + change_dir) % 4;
    direction_steps_counter *= !change_dir;
    turn_counter += change_dir;
    step_radius += change_dir * (1 - (turn_counter % 2));
  }
}

void ChunkManager::IterateChunks(int load_distance, const PositionIteratorFunc& func) const {
  // Clockwise Spiral iterator starting from center
  constexpr static int Dx[] = {1, 0, -1, 0};
  constexpr static int Dy[] = {0, 1, 0, -1};
  int direction = 0;
  int step_radius = 1;
  int direction_steps_counter = 0;
  int turn_counter = 0;
  int load_len = (load_distance) * 2 + 1;
  int num_chunk_positions = load_len * load_len;
  glm::ivec2 pos{center_.x, center_.z};

  for (int chunk_pos_idx = 0; chunk_pos_idx < num_chunk_positions; chunk_pos_idx++) {
    func(pos);
    // branchless iterate
    direction_steps_counter++;
    pos.x += Dx[direction];
    pos.y += Dy[direction];
    bool change_dir = direction_steps_counter == step_radius;
    direction = (direction + change_dir) % 4;
    direction_steps_counter *= !change_dir;
    turn_counter += change_dir;
    step_radius += change_dir * (1 - (turn_counter % 2));
  }
}

void ChunkManager::IterateChunksVertical(int load_distance,
                                         const VerticalPositionIteratorFunc& func) const {
  IterateChunks(load_distance, [&func](const glm::ivec2& pos) {
    glm::ivec3 p{pos.x, 0, pos.y};
    for (p.y = 0; p.y < NumVerticalChunks; p.y++) {
      func(p);
    }
  });
}

ChunkManager::~ChunkManager() {
  thread_pool_.wait();
  IterateChunks(load_distance_, [this](const glm::ivec2& pos) {
    glm::ivec3 p{pos.x, 0, pos.y};
    for (p.y = 0; p.y < NumVerticalChunks; p.y++) {
      chunk_map_.find_fn(
          p, [this](const std::shared_ptr<Chunk>& chunk) { FreeChunkMesh(chunk->mesh_handle); });
    }
  });
  nlohmann::json j = {{"load_distance", load_distance_}, {"frequency", frequency_}};
  SettingsManager::Get().SaveSetting(j, "chunk_manager");
}

void ChunkManager::OnImGui() {
  if (ImGui::CollapsingHeader("Chunk Manager", ImGuiTreeNodeFlags_DefaultOpen)) {
    int old_load_distance = load_distance_;
    if (ImGui::SliderInt("Load Distance", &load_distance_, 1, 72)) {
      UnloadChunksOutOfRange(old_load_distance);
      AddNewChunks(false);
    }
    ImGui::Checkbox("Greedy Meshing", &mesh_greedy_);
    ImGui::SliderFloat("Frequency", &frequency_, 0.1, 10);
    int old_lod1_distance = chunk_dist_lod_1_;
    ImGui::BeginDisabled(thread_pool_.get_tasks_running() > 0 ||
                         !chunk_mesh_finished_queue_.empty());
    if (ImGui::SliderInt("LOD 1 Distance", &chunk_dist_lod_1_, std::min(5, load_distance_ - 1),
                         load_distance_ - 1)) {
      // if greater, need to change old lod chunks into regular chunks
      // if less, need to change regular chunks into lod chunks
      if (old_lod1_distance < chunk_dist_lod_1_) {
        // change to regular chunks
        IterateChunksVertical(chunk_dist_lod_1_, [this](const glm::ivec3& pos) {
          auto chunk = chunk_map_.find(pos);
          if (chunk->lod_level != LODLevel::Regular) {
            SendChunkMeshTaskNoLOD(pos);
          }
        });
      } else {
        // change to lod chunks
        IterateChunks(chunk_dist_lod_1_, old_lod1_distance,
                      [this](const glm::ivec2& pos) { SendChunkMeshTaskLOD1(pos); });
      }
    }
    ImGui::EndDisabled();
    if (ImGui::CollapsingHeader("Stats##chunk_manager_stats", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("Center: %i %i %i", center_.x, center_.y, center_.z);
      ImGui::Text("Prev Center: %i %i %i", prev_center_.x, prev_center_.y, prev_center_.z);
      ImGui::Text("Chunk Map: %zu", chunk_map_.size());
      ImGui::Text("Chunk Mesh Queue:  %zu", chunk_mesh_queue_.size());
      ImGui::Text("Chunk Mesh Queue Imm:  %zu", chunk_mesh_queue_immediate_.size());
      ImGui::Text("Chunk Mesh Finish Queue:  %zu", chunk_mesh_finished_queue_.size());
      ImGui::Text("Chunk Terrain Queue:  %zu", chunk_terrain_queue_.size());
      ImGui::Text("Chunk Terrain Finish Queue:  %zu", finished_chunk_terrain_queue_.size());
      ImGui::Text("Chunks:  Loaded: %i, Meshed: %i Max: %i", state_stats_.loaded_chunks,
                  state_stats_.meshed_chunks, state_stats_.max_chunks);
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
    if (!chunk_map_.find_fn(
            neighbor_pos, [&neighbor_array, &off](const std::shared_ptr<Chunk>& chunk) {
              neighbor_array[ChunkNeighborOffsetToIdx(off[0], off[1], off[2])] = chunk;
            })) {
      spdlog::error("chunk not found needed for neighbor");
      if (add_new_chunks) {
        auto new_chunk = std::make_shared<Chunk>(neighbor_pos);
        chunk_map_.insert(neighbor_pos, new_chunk);
        neighbor_array[ChunkNeighborOffsetToIdx(off[0], off[1], off[2])] = new_chunk;
      }
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

void ChunkManager::FreeChunkMesh(uint32_t& handle) {
  if (handle != 0) {
    Renderer::Get().FreeStaticChunkMesh(handle);
    state_stats_.meshed_chunks--;
    handle = 0;
  }
}

void ChunkManager::SendChunkMeshTaskLOD1(const glm::ivec2& pos) {
  thread_pool_.detach_task([this, pos] {
    glm::ivec3 p;
    p.x = pos.x;
    p.z = pos.y;
    ChunkStackArray arr{};
    bool done = false;
    for (p.y = 0; p.y < NumVerticalChunks; p.y++) {
      chunk_map_.find_fn(p, [&p, &arr, &done](const std::shared_ptr<Chunk>& chunk) {
        if (chunk->lod_level == LODLevel::One) {
          done = true;
          return;
        }
        chunk->data.DownSample();
        arr[p.y] = chunk;
      });
    }
    std::vector<ChunkVertex> vertices;
    std::vector<uint32_t> indices;
    ChunkMesher mesher{block_db_.GetBlockData(), block_db_.GetMeshData()};
    mesher.GenerateLODGreedy2(arr, vertices, indices);
    std::lock_guard<std::mutex> lock(mutex_);
    lod_chunk_mesh_finished_queue_.emplace(std::move(vertices), std::move(indices), pos,
                                           LODLevel::One);
  });
}

void ChunkManager::SendChunkMeshTaskNoLOD(const glm::ivec3& pos) {
  int num_blocks = 0;
  if (!chunk_map_.find_fn(pos, [this, &num_blocks](const std::shared_ptr<Chunk>& chunk) {
        num_blocks = chunk->data.GetBlockCount();
        if (!num_blocks) {
          std::lock_guard<std::mutex> lock(mutex_);
          chunk->lod_level = LODLevel::Regular;
          chunk->mesh_state = Chunk::State::Finished;
        }
      })) {
    return;
  }
  if (!num_blocks) {
    return;
  }

  ChunkNeighborArray a;
  PopulateChunkNeighbors(a, pos, true);
  thread_pool_.detach_task([this, a, pos] {
    ChunkMesher mesher{block_db_.GetBlockData(), block_db_.GetMeshData()};
    std::vector<ChunkVertex> vertices;
    std::vector<uint32_t> indices;
    if (mesh_greedy_) {
      mesher.GenerateGreedy(a, vertices, indices);
    } else {
      mesher.GenerateSmart(a, vertices, indices);
    }
    {
      std::lock_guard<std::mutex> lock(mutex_);
      chunk_mesh_finished_queue_.emplace(std::move(vertices), std::move(indices), pos,
                                         LODLevel::Regular);
    }
  });
}

void ChunkManager::AddNewChunks(bool first_load, bool throttle) {
  // Clockwise Spiral iterator starting from center
  constexpr static int Dx[] = {1, 0, -1, 0};
  constexpr static int Dy[] = {0, 1, 0, -1};
  int direction = 0;
  int step_radius = 1;
  int direction_steps_counter = 0;
  int turn_counter = 0;
  int load_len = (load_distance_) * 2 + 1;
  int num_chunk_positions = load_len * load_len;
  glm::ivec3 pos = center_;
  pos.y = 0;

  if (!first_load) {
    int max_per_call = SettingsManager::Get().CoreCount() * 3;
    int curr = 0;
    for (int chunk_pos_idx = 0; chunk_pos_idx < num_chunk_positions; chunk_pos_idx++) {
      bool new_chunk_column = false;
      if (throttle && curr > max_per_call) break;
      for (pos.y = -1; pos.y <= NumVerticalChunks; pos.y++) {
        if (!chunk_map_.contains(pos)) {
          new_chunk_column = true;
          auto chunk = std::make_shared<Chunk>(pos);
          chunk->terrain_state = Chunk::State::Queued;
          chunk_map_.insert(pos, chunk);
        }
      }
      if (new_chunk_column) {
        chunk_terrain_queue_.emplace(pos.x, pos.z);
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
  } else {
  }
}

void ChunkManager::Init(const glm::ivec3& start_pos) {
  ZoneScoped;
  SetCenter(start_pos);
  chunk_dist_lod_1_ = std::min(5, load_distance_ - 1);
  // AddNewChunks(true);
}

bool ChunkManager::IsLoaded() const {
  return chunk_mesh_queue_.empty() && chunk_mesh_finished_queue_.empty() &&
         chunk_terrain_queue_.empty() && chunk_mesh_finished_queue_.empty() &&
         thread_pool_.get_tasks_running() == 0;
}
