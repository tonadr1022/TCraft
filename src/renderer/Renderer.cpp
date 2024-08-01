#include "Renderer.hpp"

#include <cstdint>

#include "ShaderManager.hpp"
#include "Vertex.hpp"
#include "application/SettingsManager.hpp"
#include "application/Window.hpp"
#include "pch.hpp"
#include "renderer/Material.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/Shape.hpp"
#include "renderer/opengl/Debug.hpp"
#include "renderer/opengl/Texture2dArray.hpp"
#include "resource/TextureManager.hpp"
#include "util/Paths.hpp"

Renderer* Renderer::instance_ = nullptr;

Renderer& Renderer::Get() { return *instance_; }

Renderer::Renderer() {
  EASSERT_MSG(instance_ == nullptr, "Cannot create two instances.");
  instance_ = this;
}

void Renderer::Init() {
  auto settings = SettingsManager::Get().LoadSetting("renderer");
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, nullptr);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  LoadShaders();
  wireframe_enabled_ = settings.value("wireframe_enabled", false);

  chunk_vao_.Init();
  chunk_vao_.EnableAttribute<float>(0, 3, offsetof(ChunkVertex, position));
  chunk_vao_.EnableAttribute<float>(1, 3, offsetof(ChunkVertex, tex_coords));
  // TODO: fine tune or make resizeable
  chunk_vbo_.Init(sizeof(ChunkVertex) * 100'000'00, sizeof(ChunkVertex));
  chunk_ebo_.Init(sizeof(uint32_t) * 100'000'000, sizeof(uint32_t));
  chunk_vao_.AttachVertexBuffer(chunk_vbo_.Id(), 0, 0, sizeof(ChunkVertex));
  chunk_vao_.AttachElementBuffer(chunk_ebo_.Id());
  chunk_uniform_ssbo_.Init(sizeof(ChunkDrawCmdUniform) * MaxDrawCmds, GL_DYNAMIC_STORAGE_BIT);
  chunk_draw_indirect_buffer_.Init(sizeof(DrawElementsIndirectCommand) * MaxDrawCmds,
                                   GL_DYNAMIC_STORAGE_BIT);

  reg_mesh_vao_.Init();
  reg_mesh_vao_.EnableAttribute<float>(0, 3, offsetof(Vertex, position));
  reg_mesh_vao_.EnableAttribute<float>(1, 2, offsetof(Vertex, tex_coords));
  // TODO: fine tune or make resizeable
  reg_mesh_vbo_.Init(sizeof(Vertex) * 100'000, sizeof(Vertex));
  reg_mesh_ebo_.Init(sizeof(uint32_t) * 1'000'000, sizeof(uint32_t));
  reg_mesh_vao_.AttachVertexBuffer(reg_mesh_vbo_.Id(), 0, 0, sizeof(Vertex));
  reg_mesh_vao_.AttachElementBuffer(reg_mesh_ebo_.Id());
  reg_mesh_uniform_ssbo_.Init(sizeof(DrawCmdUniform) * MaxDrawCmds / 10, GL_DYNAMIC_STORAGE_BIT);
  reg_mesh_draw_indirect_buffer_.Init(sizeof(DrawElementsIndirectCommand) * MaxDrawCmds / 10,
                                      GL_DYNAMIC_STORAGE_BIT);

  tex_materials_buffer_.Init(sizeof(TextureMaterial) * 1000, GL_DYNAMIC_STORAGE_BIT);

  std::vector<Vertex> vertices = {QuadVertices.begin(), QuadVertices.end()};
  std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};
  quad_mesh_.Allocate(vertices, indices);
}

void Renderer::Shutdown() {
  ZoneScoped;
  spdlog::info("vbo allocs: {}, ebo allocs: {}", chunk_vbo_.NumAllocs(), chunk_ebo_.NumAllocs());
  nlohmann::json settings;
  settings["wireframe_enabled"] = wireframe_enabled_;
  SettingsManager::Get().SaveSetting(settings, "renderer");
}

void Renderer::RenderWorld(const ChunkRenderParams& render_params, const RenderInfo& render_info) {
  ZoneScoped;
  {
    ZoneScopedN("Chunk render");
    SetChunkFrameDrawCommands();
    auto shader = shader_manager_.GetShader("chunk_batch");
    shader->Bind();
    shader->SetMat4("vp_matrix", render_info.vp_matrix);
    const auto& chunk_tex_arr =
        TextureManager::Get().GetTexture2dArray(render_params.chunk_tex_array_handle);
    chunk_tex_arr.Bind(0);
    chunk_vao_.Bind();
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                chunk_frame_dei_cmds_.size(), 0);
  }

  {
    ZoneScopedN("Reg Mesh render");
    SetRegMeshFrameDrawCommands();
    auto shader = shader_manager_.GetShader("single_texture");
    shader->Bind();
    shader->SetMat4("vp_matrix", render_info.vp_matrix);
    reg_mesh_vao_.Bind();
    tex_materials_buffer_.BindBase(GL_SHADER_STORAGE_BUFFER, 1);
    // spdlog::info("size {}", reg_mesh_dei_cmds_.size());
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, reg_mesh_dei_cmds_.size(),
                                0);
  }
}

void Renderer::SetRegMeshFrameDrawCommands() {
  ZoneScoped;
  reg_mesh_uniform_ssbo_.ResetOffset();
  reg_mesh_uniform_ssbo_.SubData(sizeof(DrawCmdUniform) * reg_mesh_frame_draw_cmd_uniforms_.size(),
                                 reg_mesh_frame_draw_cmd_uniforms_.data());
  // spdlog::info("reg mesh {}", reg_mesh_frame_draw_cmd_uniforms_[0].material_index);
  {
    ZoneScopedN("Clear per frame and reserve");
    reg_mesh_frame_dei_cmds_.clear();
    reg_mesh_frame_dei_cmds_.reserve(reg_mesh_frame_draw_cmd_uniforms_.size());
  }
  DrawElementsIndirectCommand cmd;
  EASSERT_MSG(reg_mesh_frame_draw_cmd_uniforms_.size() == reg_mesh_frame_draw_cmd_mesh_ids_.size(),
              "Per frame draw cmd size must equal mesh cmd size");
  for (uint32_t i = 0; i < reg_mesh_frame_draw_cmd_mesh_ids_.size(); i++) {
    const auto& draw_cmd_info = reg_mesh_dei_cmds_[reg_mesh_frame_draw_cmd_mesh_ids_[i]];
    cmd.base_instance = i;
    cmd.base_vertex = draw_cmd_info.base_vertex;
    cmd.instance_count = 1;
    cmd.count = draw_cmd_info.count;
    cmd.first_index = draw_cmd_info.first_index;
    reg_mesh_frame_dei_cmds_.emplace_back(cmd);
  }

  reg_mesh_draw_indirect_buffer_.ResetOffset();
  reg_mesh_draw_indirect_buffer_.SubData(
      sizeof(DrawElementsIndirectCommand) * reg_mesh_frame_dei_cmds_.size(),
      reg_mesh_frame_dei_cmds_.data());
  reg_mesh_draw_indirect_buffer_.Bind(GL_DRAW_INDIRECT_BUFFER);
  reg_mesh_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::SetChunkFrameDrawCommands() {
  ZoneScoped;
  chunk_uniform_ssbo_.ResetOffset();
  chunk_uniform_ssbo_.SubData(sizeof(ChunkDrawCmdUniform) * chunk_frame_draw_cmd_uniforms_.size(),
                              chunk_frame_draw_cmd_uniforms_.data());
  {
    ZoneScopedN("Clear per frame and reserve");
    chunk_frame_dei_cmds_.clear();
    chunk_frame_dei_cmds_.reserve(chunk_frame_draw_cmd_uniforms_.size());
  }
  DrawElementsIndirectCommand cmd;
  EASSERT_MSG(chunk_frame_draw_cmd_uniforms_.size() == chunk_frame_draw_cmd_mesh_ids_.size(),
              "Per frame draw cmd size must equal mesh cmd size");
  for (uint32_t i = 0; i < chunk_frame_draw_cmd_mesh_ids_.size(); i++) {
    const auto& draw_cmd_info = chunk_dei_cmds_[chunk_frame_draw_cmd_mesh_ids_[i]];
    cmd.base_instance = i;
    cmd.base_vertex = draw_cmd_info.base_vertex;
    cmd.instance_count = 1;
    cmd.count = draw_cmd_info.count;
    cmd.first_index = draw_cmd_info.first_index;
    chunk_frame_dei_cmds_.emplace_back(cmd);
  }

  chunk_draw_indirect_buffer_.ResetOffset();
  chunk_draw_indirect_buffer_.SubData(
      sizeof(DrawElementsIndirectCommand) * chunk_frame_dei_cmds_.size(),
      chunk_frame_dei_cmds_.data());
  chunk_draw_indirect_buffer_.Bind(GL_DRAW_INDIRECT_BUFFER);
  chunk_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::SubmitChunkDrawCommand(const glm::mat4& model, uint32_t mesh_handle) {
  chunk_frame_draw_cmd_mesh_ids_.emplace_back(mesh_handle);
  chunk_frame_draw_cmd_uniforms_.emplace_back(model);
}

void Renderer::SubmitRegMeshDrawCommand(const glm::mat4& model, uint32_t mesh_handle,
                                        uint32_t material_handle) {
  reg_mesh_frame_draw_cmd_mesh_ids_.emplace_back(mesh_handle);
  reg_mesh_frame_draw_cmd_uniforms_.emplace_back(model, material_allocs_[material_handle]);
}

void Renderer::SubmitQuadDrawCommand(const glm::mat4& model, uint32_t material_handle) {
  SubmitRegMeshDrawCommand(model, quad_mesh_.Handle(), material_handle);
}

uint32_t Renderer::AllocateMesh(std::vector<ChunkVertex>& vertices,
                                std::vector<uint32_t>& indices) {
  ZoneScoped;
  uint32_t chunk_vbo_offset;
  uint32_t chunk_ebo_offset;
  // uint32_t id = dei_cmds_.size();
  uint32_t id = rand();
  chunk_allocs_.try_emplace(
      id, MeshAlloc{
              .vbo_handle = chunk_vbo_.Allocate(sizeof(ChunkVertex) * vertices.size(),
                                                vertices.data(), chunk_vbo_offset),
              .ebo_handle = chunk_ebo_.Allocate(sizeof(uint32_t) * indices.size(), indices.data(),
                                                chunk_ebo_offset),
          });
  chunk_dei_cmds_.try_emplace(
      id, DrawElementsIndirectCommand{
              .count = static_cast<uint32_t>(indices.size()),
              .instance_count = 0,
              .first_index = static_cast<uint32_t>(chunk_ebo_offset / sizeof(uint32_t)),
              .base_vertex = static_cast<uint32_t>(chunk_vbo_offset / sizeof(ChunkVertex)),
              .base_instance = 0,
          });
  spdlog::info(
      "v_size: {}, e_size {}, vbo_offset: {}, ebo_offset: {}, base_vertex: {},  first_index: {},"
      "dei_size: {} vbo_allocs: {}, ebo_allocs: {}",
      sizeof(ChunkVertex) * vertices.size(), sizeof(uint32_t) * indices.size(), chunk_vbo_offset,
      chunk_ebo_offset, chunk_vbo_offset / sizeof(ChunkVertex), chunk_ebo_offset / sizeof(uint32_t),
      chunk_dei_cmds_.size(), chunk_vbo_.NumAllocs(), chunk_ebo_.NumAllocs());
  return id;
}

uint32_t Renderer::AllocateMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
  ZoneScoped;
  uint32_t vbo_offset;
  uint32_t ebo_offset;
  uint32_t id = rand();
  reg_mesh_allocs_.try_emplace(
      id, MeshAlloc{.vbo_handle = reg_mesh_vbo_.Allocate(sizeof(Vertex) * vertices.size(),
                                                         vertices.data(), vbo_offset),
                    .ebo_handle = reg_mesh_ebo_.Allocate(sizeof(uint32_t) * indices.size(),
                                                         indices.data(), ebo_offset)});
  reg_mesh_dei_cmds_.try_emplace(
      id, DrawElementsIndirectCommand{
              .count = static_cast<uint32_t>(indices.size()),
              .instance_count = 0,
              .first_index = static_cast<uint32_t>(ebo_offset / sizeof(uint32_t)),
              .base_vertex = static_cast<uint32_t>(vbo_offset / sizeof(Vertex)),
              .base_instance = 0,
          });
  return id;
}

uint32_t Renderer::AllocateMaterial(TextureMaterial& material) {
  ZoneScoped;
  uint32_t offset;
  uint32_t id = rand();
  uint32_t _ = tex_materials_buffer_.Allocate(sizeof(TextureMaterial),
                                              static_cast<void*>(&material), offset);
  material_allocs_.emplace(id, offset / sizeof(TextureMaterial));
  return id;
}

void Renderer::FreeMaterial(uint32_t material_handle) {
  ZoneScoped;
  auto it = material_allocs_.find(material_handle);
  if (it == material_allocs_.end()) {
    spdlog::error("Material with handle {} not found", material_handle);
  }
  spdlog::info("Free material {}", material_handle);
  tex_materials_buffer_.Free(it->second);
}

void Renderer::FreeRegMesh(uint32_t handle) {
  auto it = reg_mesh_allocs_.find(handle);
  if (it == reg_mesh_allocs_.end()) {
    spdlog::error("FreeRegMesh: handle not found: {}", handle);
    return;
  }
  reg_mesh_vbo_.Free(it->second.vbo_handle);
  reg_mesh_ebo_.Free(it->second.ebo_handle);
  reg_mesh_dei_cmds_.erase(it->first);
  reg_mesh_allocs_.erase(it);
}

void Renderer::FreeChunkMesh(uint32_t handle) {
  auto it = chunk_allocs_.find(handle);
  if (it == chunk_allocs_.end()) {
    spdlog::error("FreeChunk: handle not found: {}", handle);
    return;
  }
  chunk_vbo_.Free(it->second.vbo_handle);
  chunk_ebo_.Free(it->second.ebo_handle);
  chunk_dei_cmds_.erase(it->first);
  chunk_allocs_.erase(it);
}

void Renderer::StartFrame(const Window& window) {
  // TODO: don't clear and reallocate, or at least profile in the future
  chunk_frame_draw_cmd_mesh_ids_.clear();
  chunk_frame_draw_cmd_uniforms_.clear();
  reg_mesh_frame_draw_cmd_mesh_ids_.clear();
  reg_mesh_frame_draw_cmd_uniforms_.clear();

  auto dims = window.GetWindowSize();
  glViewport(0, 0, dims.x, dims.y);
  glClearColor(1, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPolygonMode(GL_FRONT_AND_BACK, wireframe_enabled_ ? GL_LINE : GL_FILL);
}

bool Renderer::OnEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_r && event.key.keysym.mod & KMOD_ALT) {
        shader_manager_.RecompileShaders();
        return true;
      }
      if (event.key.keysym.sym == SDLK_w && event.key.keysym.mod & KMOD_ALT) {
        wireframe_enabled_ = !wireframe_enabled_;
        return true;
      }
  }
  return false;
}

void Renderer::LoadShaders() {
  shader_manager_.AddShader("color", {{GET_SHADER_PATH("color.vs.glsl"), ShaderType::Vertex},
                                      {GET_SHADER_PATH("color.fs.glsl"), ShaderType::Fragment}});
  shader_manager_.AddShader("chunk_batch",
                            {{GET_SHADER_PATH("chunk_batch.vs.glsl"), ShaderType::Vertex},
                             {GET_SHADER_PATH("chunk_batch.fs.glsl"), ShaderType::Fragment}});
  shader_manager_.AddShader("single_texture",
                            {{GET_SHADER_PATH("single_texture.vs.glsl"), ShaderType::Vertex},
                             {GET_SHADER_PATH("single_texture.fs.glsl"), ShaderType::Fragment}});
}
