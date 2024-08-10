#include "Renderer.hpp"

#include <cstdint>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <tracy/Tracy.hpp>

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

namespace {
glm::mat4 CenterSizeToModel(const glm::vec2& center, const glm::vec2& size) {
  return glm::scale(glm::translate(glm::mat4{1}, {center.x, center.y, 0}), {size.x, size.y, 1});
}
}  // namespace

Renderer* Renderer::instance_ = nullptr;

Renderer& Renderer::Get() { return *instance_; }

Renderer::Renderer(Window& window) : window_(window) {
  EASSERT_MSG(instance_ == nullptr, "Cannot create two instances.");
  instance_ = this;
}

Renderer::~Renderer() { ZoneScopedN("Destroy Renderer"); }

void Renderer::ShutdownInternal() {
  spdlog::info("chunk vbo allocs: {},chunk ebo allocs: {}", chunk_vbo_.NumAllocs(),
               chunk_ebo_.NumAllocs());
  spdlog::info("reg vbo allocs: {}, reg ebo allocs: {}", reg_mesh_vbo_.NumAllocs(),
               reg_mesh_ebo_.NumAllocs());
  nlohmann::json settings;
  settings["wireframe_enabled"] = wireframe_enabled_;
  SettingsManager::Get().SaveSetting(settings, "renderer");
}

void Renderer::Init(Window& window) {
  EASSERT_MSG(instance_ == nullptr, "Can't make two instances");
  instance_ = new Renderer(window);
}

void Renderer::Shutdown() {
  ZoneScoped;
  EASSERT_MSG(instance_ != nullptr, "Can't shutdown before initializing");
  instance_->ShutdownInternal();
  // delete instance_;
}

void Renderer::Init() {
  // TODO: don't hard code size!!!!!!!!!!!!!!!!!!!!!!!
  auto settings = SettingsManager::Get().LoadSetting("renderer");
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, nullptr);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  LoadShaders();
  wireframe_enabled_ = settings.value("wireframe_enabled", false);

  uniform_ubo_.Init(sizeof(UBOUniforms), GL_DYNAMIC_STORAGE_BIT);

  chunk_vao_.Init();
  chunk_vao_.EnableAttribute<uint32_t>(0, 2, offsetof(ChunkVertex, data1));

  // TODO: fine tune or make resizeable
  chunk_vbo_.Init(sizeof(ChunkVertex) * 80'000'000, sizeof(ChunkVertex));
  chunk_ebo_.Init(sizeof(uint32_t) * 80'000'000, sizeof(uint32_t));
  chunk_vao_.AttachVertexBuffer(chunk_vbo_.Id(), 0, 0, sizeof(ChunkVertex));
  chunk_vao_.AttachElementBuffer(chunk_ebo_.Id());
  chunk_uniform_ssbo_.Init(sizeof(ChunkDrawCmdUniform) * MaxChunkDrawCmds, GL_DYNAMIC_STORAGE_BIT);
  chunk_draw_indirect_buffer_.Init(sizeof(DrawElementsIndirectCommand) * MaxChunkDrawCmds,
                                   GL_DYNAMIC_STORAGE_BIT);

  reg_mesh_vao_.Init();
  reg_mesh_vao_.EnableAttribute<float>(0, 3, offsetof(Vertex, position));
  reg_mesh_vao_.EnableAttribute<float>(1, 2, offsetof(Vertex, tex_coords));
  // TODO: fine tune or make resizeable
  reg_mesh_vbo_.Init(sizeof(Vertex) * 100'0000, sizeof(Vertex));
  reg_mesh_ebo_.Init(sizeof(uint32_t) * 1'000'000, sizeof(uint32_t));
  reg_mesh_vao_.AttachVertexBuffer(reg_mesh_vbo_.Id(), 0, 0, sizeof(Vertex));
  reg_mesh_vao_.AttachElementBuffer(reg_mesh_ebo_.Id());
  reg_mesh_uniform_ssbo_.Init(sizeof(MaterialUniforms) * MaxDrawCmds / 10, GL_DYNAMIC_STORAGE_BIT);
  reg_mesh_draw_indirect_buffer_.Init(sizeof(DrawElementsIndirectCommand) * MaxDrawCmds / 10,
                                      GL_DYNAMIC_STORAGE_BIT);

  // Quad vbo, ebo, and indirect buffer are length 1, for the quad mesh
  std::vector<Vertex> vertices = {QuadVerticesCentered.begin(), QuadVerticesCentered.end()};
  std::vector<uint32_t> indices = {0, 1, 2, 2, 1, 3};
  quad_vao_.Init();
  quad_vao_.EnableAttribute<float>(0, 3, offsetof(Vertex, position));
  quad_vao_.EnableAttribute<float>(1, 2, offsetof(Vertex, tex_coords));
  quad_vbo_.Init(sizeof(Vertex) * 4, 0, vertices.data());
  quad_ebo_.Init(sizeof(uint32_t) * 6, 0, indices.data());
  quad_vao_.AttachVertexBuffer(quad_vbo_.Id(), 0, 0, sizeof(Vertex));
  quad_vao_.AttachElementBuffer(quad_ebo_.Id());
  quad_draw_indirect_buffer_.Init(sizeof(DrawElementsIndirectCommand), GL_DYNAMIC_STORAGE_BIT);
  textured_quad_uniform_ssbo_.Init(sizeof(MaterialUniforms) * 1000, GL_DYNAMIC_STORAGE_BIT);
  quad_color_uniform_ssbo_.Init(sizeof(ColorUniforms) * 10000, GL_DYNAMIC_STORAGE_BIT);

  cube_vao_.Init();
  cube_vao_.EnableAttribute<float>(0, 3, offsetof(Vertex, position));
  cube_vao_.EnableAttribute<float>(1, 2, offsetof(Vertex, tex_coords));
  auto cube_vertices = CubeVertices;
  auto cube_indices = CubeIndices;
  cube_vbo_.Init(sizeof(cube_vertices), 0, cube_vertices.data());
  cube_ebo_.Init(sizeof(cube_indices), 0, cube_indices.data());
  cube_vao_.AttachVertexBuffer(cube_vbo_.Id(), 0, 0, sizeof(Vertex));
  cube_vao_.AttachElementBuffer(cube_ebo_.Id());

  tex_materials_buffer_.Init(sizeof(TextureMaterialData) * 1000, GL_DYNAMIC_STORAGE_BIT);
}

void Renderer::RenderQuads() {
  ZoneScoped;
  stats_.textured_quad_draw_calls += static_textured_quad_uniforms_.size();
  if (stats_.textured_quad_draw_calls == 0 && stats_.color_quad_draw_calls == 0) return;
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // UBO to ortho
  auto window_dims = window_.GetWindowSize();
  uniform_ubo_.ResetOffset();
  UBOUniforms uniform_data;
  uniform_data.vp_matrix =
      glm::ortho(0.f, static_cast<float>(window_dims.x), 0.f, static_cast<float>(window_dims.y));
  uniform_ubo_.SubData(sizeof(UBOUniforms), &uniform_data);

  quad_vao_.Bind();
  glDepthFunc(GL_ALWAYS);
  if (stats_.textured_quad_draw_calls > 0) {
    ZoneScopedN("Quad render");
    // Uses a single draw elements indirect command using instancing
    shader_manager_.GetShader("single_texture")->Bind();
    textured_quad_uniform_ssbo_.ResetOffset();
    if (static_textured_quad_uniforms_.size()) {
      textured_quad_uniform_ssbo_.SubData(
          sizeof(MaterialUniforms) * static_textured_quad_uniforms_.size(),
          static_textured_quad_uniforms_.data());
    }
    if (quad_textured_uniforms_.size()) {
      textured_quad_uniform_ssbo_.SubData(sizeof(MaterialUniforms) * quad_textured_uniforms_.size(),
                                          quad_textured_uniforms_.data());
    }
    textured_quad_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
    tex_materials_buffer_.BindBase(GL_SHADER_STORAGE_BUFFER, 1);
    DrawElementsIndirectCommand cmd{
        .count = 6,
        .instance_count = stats_.textured_quad_draw_calls,
        .first_index = 0,
        .base_vertex = 0,
        .base_instance = 0,
    };
    quad_draw_indirect_buffer_.ResetOffset();
    quad_draw_indirect_buffer_.SubData(sizeof(DrawElementsIndirectCommand), &cmd);
    quad_draw_indirect_buffer_.Bind(GL_DRAW_INDIRECT_BUFFER);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 1, 0);
  }
  // TODO: static color quads
  if (stats_.color_quad_draw_calls > 0) {
    shader_manager_.GetShader("color")->Bind();
    quad_color_uniform_ssbo_.ResetOffset();
    quad_color_uniform_ssbo_.SubData(sizeof(ColorUniforms) * quad_color_uniforms_.size(),
                                     quad_color_uniforms_.data());
    quad_color_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
    DrawElementsIndirectCommand cmd{
        .count = 6,
        .instance_count = stats_.color_quad_draw_calls,
        .first_index = 0,
        .base_vertex = 0,
        .base_instance = 0,
    };
    quad_draw_indirect_buffer_.ResetOffset();
    quad_draw_indirect_buffer_.SubData(sizeof(DrawElementsIndirectCommand), &cmd);
    quad_draw_indirect_buffer_.Bind(GL_DRAW_INDIRECT_BUFFER);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 1, 0);
  }
  glDisable(GL_BLEND);
  glDepthFunc(GL_LESS);
  quad_textured_uniforms_.clear();
  quad_color_uniforms_.clear();
}

void Renderer::RenderRegMeshes() {}

void Renderer::RenderWorld(const ChunkRenderParams& render_params, const RenderInfo& render_info) {
  ZoneScoped;
  uniform_ubo_.ResetOffset();
  uniform_ubo_.BindBase(GL_UNIFORM_BUFFER, 0);
  UBOUniforms uniform_data;
  uniform_data.vp_matrix = render_info.vp_matrix;

  uniform_ubo_.SubData(sizeof(UBOUniforms), &uniform_data);
  if (chunk_frame_draw_cmd_uniforms_.size() > 0) {
    ZoneScopedN("Chunk render");
    chunk_uniform_ssbo_.ResetOffset();
    chunk_uniform_ssbo_.SubData(sizeof(ChunkDrawCmdUniform) * chunk_frame_draw_cmd_uniforms_.size(),
                                chunk_frame_draw_cmd_uniforms_.data());
    chunk_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
    SetMeshFrameDrawCommands(chunk_draw_indirect_buffer_, chunk_frame_draw_cmd_uniforms_,
                             chunk_frame_draw_cmd_mesh_ids_, chunk_frame_dei_cmds_,
                             chunk_dei_cmds_);
    shader_manager_.GetShader(render_params.shader_name)->Bind();
    const auto& chunk_tex_arr =
        TextureManager::Get().GetTexture2dArray(render_params.chunk_tex_array_handle);
    chunk_tex_arr.Bind(0);
    chunk_vao_.Bind();
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                chunk_frame_dei_cmds_.size(), 0);
  }

  if (reg_mesh_frame_draw_cmd_uniforms_.size() > 0) {
    ZoneScopedN("Reg Mesh render");
    reg_mesh_uniform_ssbo_.ResetOffset();
    reg_mesh_uniform_ssbo_.SubData(
        sizeof(MaterialUniforms) * reg_mesh_frame_draw_cmd_uniforms_.size(),
        reg_mesh_frame_draw_cmd_uniforms_.data());
    reg_mesh_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
    SetMeshFrameDrawCommands(reg_mesh_draw_indirect_buffer_, reg_mesh_frame_draw_cmd_uniforms_,
                             reg_mesh_frame_draw_cmd_mesh_ids_, reg_mesh_frame_dei_cmds_,
                             reg_mesh_dei_cmds_);
    auto shader = shader_manager_.GetShader("single_texture");
    shader->Bind();
    reg_mesh_vao_.Bind();
    tex_materials_buffer_.BindBase(GL_SHADER_STORAGE_BUFFER, 1);
    // spdlog::info("size {}", reg_mesh_dei_cmds_.size());
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                reg_mesh_frame_dei_cmds_.size(), 0);
  }
  RenderQuads();
}

template <typename UniformType>
void Renderer::SetMeshFrameDrawCommands(
    Buffer& draw_indirect_buffer, std::vector<UniformType>& frame_draw_cmd_uniforms,
    std::vector<uint32_t>& frame_draw_cmd_mesh_ids,
    std::vector<DrawElementsIndirectCommand>& frame_dei_cmds,
    std::unordered_map<uint32_t, DrawElementsIndirectCommand>& dei_cmds) {
  ZoneScoped;
  {
    ZoneScopedN("Clear per frame and reserve");
    frame_dei_cmds.clear();
    frame_dei_cmds.reserve(frame_draw_cmd_uniforms.size());
  }
  DrawElementsIndirectCommand cmd;
  EASSERT_MSG(frame_draw_cmd_uniforms.size() == frame_draw_cmd_mesh_ids.size(),
              "Per frame draw cmd size must equal mesh cmd size");
  for (uint32_t i = 0; i < frame_draw_cmd_mesh_ids.size(); i++) {
    const auto& draw_cmd_info = dei_cmds[frame_draw_cmd_mesh_ids[i]];
    cmd.base_instance = i;
    cmd.base_vertex = draw_cmd_info.base_vertex;
    cmd.instance_count = 1;
    cmd.count = draw_cmd_info.count;
    cmd.first_index = draw_cmd_info.first_index;
    frame_dei_cmds.emplace_back(cmd);
  }
  draw_indirect_buffer.ResetOffset();
  draw_indirect_buffer.SubData(sizeof(DrawElementsIndirectCommand) * frame_dei_cmds.size(),
                               frame_dei_cmds.data());
  draw_indirect_buffer.Bind(GL_DRAW_INDIRECT_BUFFER);
}

void Renderer::SubmitChunkDrawCommand(const glm::mat4& model, uint32_t mesh_handle) {
  ZoneScoped;
  chunk_frame_draw_cmd_mesh_ids_.emplace_back(mesh_handle);
  chunk_frame_draw_cmd_uniforms_.emplace_back(model);
}

void Renderer::SubmitRegMeshDrawCommand(const glm::mat4& model, uint32_t mesh_handle,
                                        uint32_t material_handle) {
  ZoneScoped;
  reg_mesh_frame_draw_cmd_mesh_ids_.emplace_back(mesh_handle);
  reg_mesh_frame_draw_cmd_uniforms_.emplace_back(model, material_allocs_[material_handle]);
}

void Renderer::DrawQuad(uint32_t material_handle, const glm::vec2& center, const glm::vec2& size) {
  // TODO: handle gracefully no material handle
  quad_textured_uniforms_.emplace_back(CenterSizeToModel(center, size),
                                       material_allocs_.at(material_handle));
  stats_.textured_quad_draw_calls++;
}

void Renderer::DrawQuad(const glm::vec3& color, const glm::vec2& center, const glm::vec2& size) {
  quad_color_uniforms_.emplace_back(CenterSizeToModel(center, size), color);
  stats_.color_quad_draw_calls++;
}

void Renderer::AddStaticQuad(uint32_t material_handle, const glm::vec2& center,
                             const glm::vec2& size) {
  static_textured_quad_uniforms_.emplace_back(CenterSizeToModel(center, size),
                                              material_allocs_.at(material_handle));
}

uint32_t Renderer::AllocateMesh(std::vector<ChunkVertex>& vertices,
                                std::vector<uint32_t>& indices) {
  ZoneScoped;
  uint32_t chunk_vbo_offset;
  uint32_t chunk_ebo_offset;
  // uint32_t id = dei_cmds_.size();
  uint32_t id = rand();
  uint32_t vbo_handle;
  uint32_t ebo_handle;
  {
    ZoneScopedN("vbo alloc");
    vbo_handle = chunk_vbo_.Allocate(sizeof(ChunkVertex) * vertices.size(), vertices.data(),
                                     chunk_vbo_offset);
  }
  {
    ZoneScopedN("ebo alloc");
    ebo_handle =
        chunk_ebo_.Allocate(sizeof(uint32_t) * indices.size(), indices.data(), chunk_ebo_offset);
  }

  chunk_allocs_.emplace(id, MeshAlloc{
                                .vbo_handle = vbo_handle,
                                .ebo_handle = ebo_handle,
                            });
  chunk_dei_cmds_.emplace(
      id, DrawElementsIndirectCommand{
              .count = static_cast<uint32_t>(indices.size()),
              .instance_count = 0,
              .first_index = static_cast<uint32_t>(chunk_ebo_offset / sizeof(uint32_t)),
              .base_vertex = static_cast<uint32_t>(chunk_vbo_offset / sizeof(ChunkVertex)),
              .base_instance = 0,
          });
  // spdlog::info(
  //     "v_size: {}, e_size {}, vbo_offset: {}, ebo_offset: {}, base_vertex: {},  first_index: {},"
  //     "dei_size: {} vbo_allocs: {}, ebo_allocs: {}",
  //     sizeof(ChunkVertex) * vertices.size(), sizeof(uint32_t) * indices.size(), chunk_vbo_offset,
  //     chunk_ebo_offset, chunk_vbo_offset / sizeof(ChunkVertex), chunk_ebo_offset /
  //     sizeof(uint32_t), chunk_dei_cmds_.size(), chunk_vbo_.NumAllocs(), chunk_ebo_.NumAllocs());
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

uint32_t Renderer::AllocateMaterial(TextureMaterialData& material) {
  ZoneScoped;
  uint32_t offset;
  uint32_t handle = tex_materials_buffer_.Allocate(sizeof(TextureMaterialData),
                                                   static_cast<void*>(&material), offset);
  material_allocs_.emplace(handle, offset / sizeof(TextureMaterialData));
  return handle;
}

void Renderer::FreeMaterial(uint32_t material_handle) {
  ZoneScoped;
  auto it = material_allocs_.find(material_handle);
  if (it == material_allocs_.end()) {
    spdlog::error("Material with handle {} not found", material_handle);
  }
  tex_materials_buffer_.Free(material_handle);
  material_allocs_.erase(it);
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

void Renderer::ClearStaticData() {
  static_textured_quad_uniforms_.clear();
  stats_.textured_quad_draw_calls = 0;
}

void Renderer::StartFrame(const Window& window) {
  {
    ZoneScopedN("clear buffers");
    // TODO: don't clear and reallocate, or at least profile in the future
    chunk_frame_draw_cmd_mesh_ids_.clear();
    chunk_frame_draw_cmd_uniforms_.clear();
    reg_mesh_frame_draw_cmd_mesh_ids_.clear();
    reg_mesh_frame_draw_cmd_uniforms_.clear();
    stats_ = {};
  }

  {
    ZoneScopedN("OpenGL state");
    auto dims = window.GetWindowSize();
    glViewport(0, 0, dims.x, dims.y);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_enabled_ ? GL_LINE : GL_FILL);
  }
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
  shader_manager_.AddShader("chunk_batch_block",
                            {{GET_SHADER_PATH("chunk_batch_block.vs.glsl"), ShaderType::Vertex},
                             {GET_SHADER_PATH("chunk_batch_block.fs.glsl"), ShaderType::Fragment}});
  shader_manager_.AddShader("chunk_batch",
                            {{GET_SHADER_PATH("chunk_batch.vs.glsl"), ShaderType::Vertex},
                             {GET_SHADER_PATH("chunk_batch.fs.glsl"), ShaderType::Fragment}});
  shader_manager_.AddShader("single_texture",
                            {{GET_SHADER_PATH("single_texture.vs.glsl"), ShaderType::Vertex},
                             {GET_SHADER_PATH("single_texture.fs.glsl"), ShaderType::Fragment}});
  shader_manager_.AddShader("block_outline",
                            {{GET_SHADER_PATH("block_outline.vs.glsl"), ShaderType::Vertex},
                             {GET_SHADER_PATH("block_outline.gs.glsl"), ShaderType::Geometry},
                             {GET_SHADER_PATH("block_outline.fs.glsl"), ShaderType::Fragment}});
}

void Renderer::DrawBlockOutline(const glm::vec3& block_pos, const glm::mat4& view,
                                const glm::mat4& projection) {
  glm::mat4 model = glm::translate(
      glm::scale(glm::translate(glm::mat4{1.f}, block_pos), glm::vec3(1.005f)), glm::vec3(-.0025f));
  auto shader = shader_manager_.GetShader("block_outline");
  shader->Bind();
  shader->SetMat4("model", model);
  shader->SetMat4("view", view);
  shader->SetMat4("projection", projection);
  shader->SetFloat("line_width", .01f);
  cube_vao_.Bind();
  glDrawElements(GL_LINES, CubeIndices.size(), GL_UNSIGNED_INT, nullptr);
}
