#include "Renderer.hpp"

#include <SDL_timer.h>
#include <imgui.h>

#include <cstdint>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <tracy/Tracy.hpp>

#include "ShaderManager.hpp"
#include "Vertex.hpp"
#include "application/SettingsManager.hpp"
#include "application/Window.hpp"
#include "gameplay/world/ChunkDef.hpp"
#include "pch.hpp"
#include "renderer/Frustum.hpp"
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
  spdlog::info("chunk vbo allocs: {},chunk ebo allocs: {}", static_chunk_vbo_.NumActiveAllocs(),
               static_chunk_ebo_.NumActiveAllocs());
  spdlog::info("reg vbo allocs: {}, reg ebo allocs: {}", reg_mesh_vbo_.NumActiveAllocs(),
               reg_mesh_ebo_.NumActiveAllocs());
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

void Renderer::InitFrameBuffers() {
  auto dims = Window::Get().GetWindowSize();
  if (fbo1_tex_) {
    glDeleteTextures(1, &fbo1_tex_);
  }
  glCreateTextures(GL_TEXTURE_2D, 1, &fbo1_tex_);
  glTextureStorage2D(fbo1_tex_, 1, GL_RGBA8, dims.x, dims.y);
  glTextureParameteri(fbo1_tex_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(fbo1_tex_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // if (fbo1_depth_tex_) {
  //   glDeleteTextures(1, &fbo1_depth_tex_);
  // }
  // glCreateTextures(GL_TEXTURE_2D, 1, &fbo1_depth_tex_);
  // glTextureStorage2D(fbo1_depth_tex_, 1, GL_DEPTH24_STENCIL8, dims.x, dims.y);

  if (rbo1_) {
    glDeleteRenderbuffers(1, &rbo1_);
  }
  glCreateRenderbuffers(1, &rbo1_);
  glNamedRenderbufferStorage(rbo1_, GL_DEPTH24_STENCIL8, dims.x, dims.y);

  if (fbo1_) {
    glDeleteFramebuffers(1, &fbo1_);
  }
  glCreateFramebuffers(1, &fbo1_);
  glNamedFramebufferTexture(fbo1_, GL_COLOR_ATTACHMENT0, fbo1_tex_, 0);
  glNamedFramebufferRenderbuffer(fbo1_, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo1_);

  if (glCheckNamedFramebufferStatus(fbo1_, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    spdlog::error("Frambuffer incomplete");
  }
}

void Renderer::Init() {
  // TODO: don't hard code size!!!!!!!!!!!!!!!!!!!!!!!
  auto settings = SettingsManager::Get().LoadSetting("renderer");
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_MULTISAMPLE);
  glDebugMessageCallback(MessageCallback, nullptr);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  LoadShaders();
  wireframe_enabled_ = settings.value("wireframe_enabled", false);

  uniform_ubo_.Init(sizeof(UBOUniforms), GL_DYNAMIC_STORAGE_BIT);

  static_chunk_vao_.Init();
  static_chunk_vao_.EnableAttribute<uint32_t>(0, 2, offsetof(ChunkVertex, data1));
  // TODO: fine tune or make resizeable
  static_chunk_vbo_.Init(sizeof(ChunkVertex) * 80'000'000, sizeof(ChunkVertex));
  static_chunk_ebo_.Init(UINT32_MAX / 2, sizeof(uint32_t));
  static_chunk_vao_.AttachVertexBuffer(static_chunk_vbo_.Id(), 0, 0, sizeof(ChunkVertex));
  static_chunk_vao_.AttachElementBuffer(static_chunk_ebo_.Id());
  static_chunk_draw_count_buffer_.Init(sizeof(GLuint), 0, nullptr);

  lod_static_chunk_vao_.Init();
  lod_static_chunk_vao_.EnableAttribute<uint32_t>(0, 2, offsetof(ChunkVertex, data1));
  // TODO: fine tune or make resizeable
  lod_static_chunk_vbo_.Init(sizeof(ChunkVertex) * 80'000'000, sizeof(ChunkVertex));
  lod_static_chunk_ebo_.Init(UINT32_MAX / 2, sizeof(uint32_t));
  lod_static_chunk_vao_.AttachVertexBuffer(lod_static_chunk_vbo_.Id(), 0, 0, sizeof(ChunkVertex));
  lod_static_chunk_vao_.AttachElementBuffer(lod_static_chunk_ebo_.Id());
  lod_static_chunk_draw_count_buffer_.Init(sizeof(GLuint), 0, nullptr);

  chunk_vao_.Init();
  chunk_vao_.EnableAttribute<uint32_t>(0, 2, offsetof(ChunkVertex, data1));
  // TODO: fine tune or make resizeable
  chunk_vbo_.Init(sizeof(ChunkVertex) * 80, sizeof(ChunkVertex), sizeof(ChunkVertex) * 100'000'000);
  chunk_ebo_.Init(sizeof(uint32_t) * 80'000'0, sizeof(uint32_t));
  chunk_vao_.AttachVertexBuffer(chunk_vbo_.Id(), 0, 0, sizeof(ChunkVertex));
  chunk_vao_.AttachElementBuffer(chunk_ebo_.Id());

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

  full_screen_quad_vao_.Init();
  full_screen_quad_vao_.EnableAttribute<float>(0, 3, offsetof(Vertex, position));
  full_screen_quad_vao_.EnableAttribute<float>(1, 2, offsetof(Vertex, tex_coords));
  vertices = {QuadVerticesFull.begin(), QuadVerticesFull.end()};
  full_screen_quad_vbo_.Init(sizeof(Vertex) * 4, 0, vertices.data());
  full_screen_quad_ebo_.Init(sizeof(uint32_t) * 6, 0, indices.data());
  full_screen_quad_vao_.AttachVertexBuffer(full_screen_quad_vbo_.Id(), 0, 0, sizeof(Vertex));
  full_screen_quad_vao_.AttachElementBuffer(full_screen_quad_ebo_.Id());

  // TODO: track quad count and figure out better numbers, or make resizable if draw too many
  textured_quad_uniform_ssbo_.Init(sizeof(MaterialUniforms) * 1000, GL_DYNAMIC_STORAGE_BIT);
  color_uniform_ssbo_.Init(sizeof(ColorUniforms) * 10000, GL_DYNAMIC_STORAGE_BIT);

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

  skybox_vao_.Init();
  skybox_vao_.EnableAttribute<float>(0, 3, 0);
  skybox_vbo_.Init(sizeof(CubePositionVertices), 0, CubePositionVertices);
  skybox_vao_.AttachVertexBuffer(skybox_vbo_.Id(), 0, 0, sizeof(float) * 3);

  InitFrameBuffers();
}

void Renderer::DrawQuads() {
  ZoneScoped;
  stats_.textured_quad_draw_calls += static_textured_quad_uniforms_.size();
  if (stats_.textured_quad_draw_calls == 0 && stats_.color_quad_draw_calls == 0) return;
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // change UBO to ortho
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
    ShaderManager::Get().GetShader("single_texture")->Bind();
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
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                            stats_.textured_quad_draw_calls);
  }
  // TODO: static color quads
  if (stats_.color_quad_draw_calls > 0) {
    ShaderManager::Get().GetShader("color")->Bind();
    color_uniform_ssbo_.ResetOffset();
    color_uniform_ssbo_.SubData(sizeof(ColorUniforms) * quad_color_uniforms_.size(),
                                quad_color_uniforms_.data());
    color_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                            stats_.color_quad_draw_calls);
  }
  glDisable(GL_BLEND);
  glDepthFunc(GL_LESS);
}

void Renderer::DrawLines(const ChunkRenderParams&, const RenderInfo&) {
  if (lines_frame_uniforms_mesh_ids_.first.empty() &&
      lines_no_depth_frame_uniforms_mesh_ids_.first.empty())
    return;
  auto shader = ShaderManager::Get().GetShader("color");
  shader->Bind();
  reg_mesh_vao_.Bind();
  auto draw_lines_batch =
      [this](std::pair<std::vector<ColorUniforms>, std::vector<uint32_t>>& uniforms_mesh_ids) {
        if (uniforms_mesh_ids.first.empty()) return;
        color_uniform_ssbo_.ResetOffset();
        color_uniform_ssbo_.SubData(sizeof(ColorUniforms) * uniforms_mesh_ids.first.size(),
                                    uniforms_mesh_ids.first.data());
        color_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
        // TODO: make only one frame dei cmds vector
        SetDrawIndirectBufferData(reg_mesh_draw_indirect_buffer_, uniforms_mesh_ids.second,
                                  frame_dei_cmd_vec_, reg_mesh_dei_cmds_);
        reg_mesh_draw_indirect_buffer_.Bind(GL_DRAW_INDIRECT_BUFFER);
        glMultiDrawElementsIndirect(GL_LINES, GL_UNSIGNED_INT, nullptr, frame_dei_cmd_vec_.size(),
                                    0);
      };
  draw_lines_batch(lines_frame_uniforms_mesh_ids_);

  if (!lines_no_depth_frame_uniforms_mesh_ids_.first.empty()) {
    glDisable(GL_DEPTH_TEST);
    draw_lines_batch(lines_no_depth_frame_uniforms_mesh_ids_);
    glEnable(GL_DEPTH_TEST);
  }
}

void Renderer::DrawStaticChunks(const ChunkRenderParams& render_params,
                                const RenderInfo& render_info) {
  ZoneScoped;
  bool frustum_shader_set = false;
  auto set_frustum_shader_data = [this, &frustum_shader_set, &render_info](Shader& cull_shader) {
    cull_shader.Bind();
    if (frustum_shader_set) return;

    frustum_shader_set = true;
    Frustum frustum;
    if (settings.extra_fov_degrees > 0) {
      auto aspect_ratio = Window::Get().GetAspectRatio();
      frustum.SetData(glm::perspective(glm::radians(SettingsManager::Get().fps_cam_fov_deg +
                                                    settings.extra_fov_degrees + 180),
                                       aspect_ratio, 0.01f, 3000.f) *
                      render_info.view_matrix);
    } else {
      frustum.SetData(render_info.vp_matrix);
    }
    cull_shader.SetVec3("u_view_pos", render_info.view_pos);
    cull_shader.SetBool("u_cull_frustum", settings.cull_frustum);
    cull_shader.SetFloat("u_min_cull_dist", settings.chunk_cull_distance_min);
    cull_shader.SetFloat("u_max_cull_dist", settings.chunk_cull_distance_max);
    // A UBO could be used, but this is more straightforward
    const auto& frustum_data = frustum.GetData();
    cull_shader.SetVec4("plane0", frustum_data[0]);
    cull_shader.SetVec4("plane1", frustum_data[1]);
    cull_shader.SetVec4("plane2", frustum_data[2]);
    cull_shader.SetVec4("plane3", frustum_data[3]);
    cull_shader.SetVec4("plane4", frustum_data[4]);
    cull_shader.SetVec4("plane5", frustum_data[5]);
  };
  // only render if there are static chunks
  if (!static_chunk_allocs_.empty()) {
    // if an allocation change happend this frame (alloc or free), reset the buffers
    if (static_chunk_buffer_dirty_) {
      static_chunk_buffer_dirty_ = false;
      static_chunk_draw_info_buffer_.Init(
          static_chunk_vbo_.Allocs().size() * static_chunk_vbo_.AllocSize(), GL_DYNAMIC_STORAGE_BIT,
          static_chunk_vbo_.Allocs().data());
      static_chunk_draw_indirect_buffer_.Init(
          static_chunk_vbo_.NumActiveAllocs() * sizeof(DrawElementsIndirectCommand),
          GL_DYNAMIC_STORAGE_BIT);
      static_chunk_uniform_ssbo_.Init(
          static_chunk_vbo_.Allocs().size() * sizeof(StaticChunkDrawCmdUniform),
          GL_DYNAMIC_STORAGE_BIT, nullptr);
    }
    // No blending for opaque. TODO: transparent chunks
    glBlendFunc(GL_ONE, GL_ZERO);

    // reset the index counter for cull compute shader
    uint32_t zero{0};
    glClearNamedBufferSubData(static_chunk_draw_count_buffer_.Id(), GL_R32UI, 0, sizeof(GLuint),
                              GL_RED, GL_UNSIGNED_INT, &zero);

    // bind and generate indirect draw commands and uniform buffers with compute shader
    auto cull_shader = ShaderManager::Get().GetShader("chunk_cull");
    set_frustum_shader_data(cull_shader.value());

    static_chunk_draw_info_buffer_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
    static_chunk_draw_indirect_buffer_.BindBase(GL_SHADER_STORAGE_BUFFER, 1);
    static_chunk_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 2);
    static_chunk_draw_count_buffer_.BindBase(GL_SHADER_STORAGE_BUFFER, 3);
    // Round work groups up by one, and it's a one dimensional work load
    glDispatchCompute((static_chunk_vbo_.Allocs().size() + 63) / 64, 1, 1);
    // Wait for data to be written to
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // auto* cnt =
    //     (GLuint*)glMapNamedBufferRange(static_chunk_draw_count_buffer_.Id(), 0, sizeof(GLuint),
    //                                    GL_MAP_READ_BIT | GL_MAP_COHERENT_BIT);
    // if (cnt) {
    //   spdlog::info("{}", *cnt);
    //   glUnmapNamedBuffer(static_chunk_draw_count_buffer_.Id());
    // }

    // draw using the uniforms and indirect buffers
    auto chunk_shader = ShaderManager::Get().GetShader("chunk_batch");
    chunk_shader->Bind();
    chunk_shader->SetBool("u_UseTexture", settings.chunk_render_use_texture);
    chunk_shader->SetBool("u_UseAO", settings.chunk_use_ao);

    const auto& chunk_tex_arr =
        TextureManager::Get().GetTexture2dArray(render_params.chunk_tex_array_handle);
    chunk_tex_arr.Bind(0);
    static_chunk_vao_.Bind();
    static_chunk_draw_indirect_buffer_.Bind(GL_DRAW_INDIRECT_BUFFER);
    static_chunk_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
    static_chunk_draw_count_buffer_.Bind(GL_PARAMETER_BUFFER);
    // https://registry.khronos.org/OpenGL/extensions/ARB/ARB_indirect_parameters.txt
    glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 0,
                                     static_chunk_vbo_.NumActiveAllocs(),
                                     sizeof(DrawElementsIndirectCommand));
  }

  if (!lod_static_chunk_allocs_.empty()) {
    // if an allocation change happend this frame (alloc or free), reset the buffers
    if (lod_static_chunk_buffer_dirty_) {
      lod_static_chunk_buffer_dirty_ = false;
      lod_static_chunk_draw_info_buffer_.Init(
          lod_static_chunk_vbo_.Allocs().size() * lod_static_chunk_vbo_.AllocSize(),
          GL_DYNAMIC_STORAGE_BIT, lod_static_chunk_vbo_.Allocs().data());
      lod_static_chunk_draw_indirect_buffer_.Init(
          lod_static_chunk_vbo_.NumActiveAllocs() * sizeof(DrawElementsIndirectCommand),
          GL_DYNAMIC_STORAGE_BIT);
      lod_static_chunk_uniform_ssbo_.Init(
          lod_static_chunk_vbo_.Allocs().size() * sizeof(StaticChunkDrawCmdUniform),
          GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT,
          nullptr);
    }

    // reset the index counter for cull compute shader
    uint32_t zero{0};
    glClearNamedBufferSubData(lod_static_chunk_draw_count_buffer_.Id(), GL_R32UI, 0, sizeof(GLuint),
                              GL_RED, GL_UNSIGNED_INT, &zero);

    auto cull_shader = ShaderManager::Get().GetShader("chunk_cull");
    set_frustum_shader_data(cull_shader.value());

    lod_static_chunk_draw_info_buffer_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
    lod_static_chunk_draw_indirect_buffer_.BindBase(GL_SHADER_STORAGE_BUFFER, 1);
    lod_static_chunk_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 2);
    lod_static_chunk_draw_count_buffer_.BindBase(GL_SHADER_STORAGE_BUFFER, 3);
    // Round work groups up by one, and it's a one dimensional work load
    glDispatchCompute((lod_static_chunk_vbo_.Allocs().size() + 63) / 64, 1, 1);
    // Wait for data to be written to
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // draw using the uniforms and indirect buffers
    auto chunk_shader = ShaderManager::Get().GetShader("lod_chunk_batch");
    chunk_shader->Bind();
    chunk_shader->SetBool("u_UseTexture", settings.chunk_render_use_texture);

    const auto& chunk_tex_arr =
        TextureManager::Get().GetTexture2dArray(render_params.chunk_tex_array_handle);
    chunk_tex_arr.Bind(0);
    lod_static_chunk_vao_.Bind();
    lod_static_chunk_draw_indirect_buffer_.Bind(GL_DRAW_INDIRECT_BUFFER);
    lod_static_chunk_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
    lod_static_chunk_draw_count_buffer_.Bind(GL_PARAMETER_BUFFER);
    // https://registry.khronos.org/OpenGL/extensions/ARB/ARB_indirect_parameters.txt
    glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 0,
                                     lod_static_chunk_vbo_.NumActiveAllocs(),
                                     sizeof(DrawElementsIndirectCommand));
  }
}

void Renderer::DrawNonStaticChunks(const ChunkRenderParams& render_params, const RenderInfo&) {
  ZoneScoped;
  if (chunk_frame_uniforms_mesh_ids_.first.empty()) return;
  chunk_uniform_ssbo_.ResetOffset();
  chunk_uniform_ssbo_.SubData(
      sizeof(ChunkDrawCmdUniform) * chunk_frame_uniforms_mesh_ids_.first.size(),
      chunk_frame_uniforms_mesh_ids_.first.data());
  chunk_frame_dei_cmds_.clear();
  chunk_frame_dei_cmds_.reserve(chunk_frame_uniforms_mesh_ids_.first.size());
  DrawElementsIndirectCommand cmd;
  for (uint32_t i = 0; i < chunk_frame_uniforms_mesh_ids_.first.size(); i++) {
    const auto& draw_cmd_info = chunk_dei_cmds_[chunk_frame_uniforms_mesh_ids_.second[i]];
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

  chunk_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
  chunk_draw_indirect_buffer_.Bind(GL_DRAW_INDIRECT_BUFFER);

  ShaderManager::Get().GetShader(render_params.shader_name)->Bind();
  const auto& chunk_tex_arr =
      TextureManager::Get().GetTexture2dArray(render_params.chunk_tex_array_handle);
  chunk_tex_arr.Bind(0);
  chunk_vao_.Bind();
  glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, chunk_frame_dei_cmds_.size(),
                              0);
}

void Renderer::DrawRegularMeshes(const ChunkRenderParams&, const RenderInfo&) {
  if (reg_mesh_frame_uniforms_mesh_ids_.first.empty()) return;
  ZoneScopedN("Reg Mesh render");
  reg_mesh_uniform_ssbo_.ResetOffset();
  reg_mesh_uniform_ssbo_.SubData(
      sizeof(MaterialUniforms) * reg_mesh_frame_uniforms_mesh_ids_.first.size(),
      reg_mesh_frame_uniforms_mesh_ids_.first.data());
  reg_mesh_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
  // TODO: make only one frame dei cmds vector
  SetDrawIndirectBufferData(reg_mesh_draw_indirect_buffer_,
                            reg_mesh_frame_uniforms_mesh_ids_.second, frame_dei_cmd_vec_,
                            reg_mesh_dei_cmds_);
  reg_mesh_draw_indirect_buffer_.Bind(GL_DRAW_INDIRECT_BUFFER);
  auto shader = ShaderManager::Get().GetShader("single_texture");
  shader->Bind();
  reg_mesh_vao_.Bind();
  tex_materials_buffer_.BindBase(GL_SHADER_STORAGE_BUFFER, 1);
  glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, frame_dei_cmd_vec_.size(), 0);
}

void Renderer::DrawLine(const glm::mat4& model, const glm::vec3& color, uint32_t mesh_handle,
                        bool ignore_depth) {
  auto& uniforms_ids =
      ignore_depth ? lines_no_depth_frame_uniforms_mesh_ids_ : lines_frame_uniforms_mesh_ids_;
  uniforms_ids.first.emplace_back(model, color);
  uniforms_ids.second.emplace_back(mesh_handle);
}

void Renderer::Render(const ChunkRenderParams& render_params, const RenderInfo& render_info) {
  ZoneScoped;
  UBOUniforms uniform_data;
  uniform_data.vp_matrix = render_info.vp_matrix;
  uniform_ubo_.ResetOffset();
  uniform_ubo_.SubData(sizeof(UBOUniforms), &uniform_data);
  uniform_ubo_.BindBase(GL_UNIFORM_BUFFER, 0);
  // TODO: only on event
  auto dims = Window::Get().GetWindowSize();
  glViewport(0, 0, dims.x, dims.y);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo1_);
  glEnable(GL_STENCIL_TEST);
  glEnable(GL_DEPTH_TEST);
  glClearColor(0.1, 0.1, 0.1, 0.1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPolygonMode(GL_FRONT_AND_BACK, wireframe_enabled_ ? GL_LINE : GL_FILL);

  // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  DrawStaticChunks(render_params, render_info);
  DrawNonStaticChunks(render_params, render_info);
  DrawRegularMeshes(render_params, render_info);
  DrawLines(render_params, render_info);

  static const std::array<std::string, 6> Sky1Strings = {
      GET_TEXTURE_PATH("skybox1/px.png"), GET_TEXTURE_PATH("skybox1/nx.png"),
      GET_TEXTURE_PATH("skybox1/py.png"), GET_TEXTURE_PATH("skybox1/ny.png"),
      GET_TEXTURE_PATH("skybox1/pz.png"), GET_TEXTURE_PATH("skybox1/nz.png"),
  };

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDepthFunc(GL_LEQUAL);
  skybox_vao_.Bind();
  // TODO: rename to sky and make more parameters, and split off into separate form main render call
  auto skybox_shader = ShaderManager::Get().GetShader("skybox").value();
  skybox_shader.Bind();
  skybox_shader.SetMat4("vp_matrix",
                        render_info.proj_matrix * glm::mat4(glm::mat3(render_info.view_matrix)));

  double curr_time = SDL_GetPerformanceCounter() * .000000001;
  glm::vec3 sun_dir = glm::normalize(glm::vec3{glm::cos(curr_time), glm::sin(curr_time), 0});
  skybox_shader.SetVec3("u_sun_direction", sun_dir);
  skybox_shader.SetFloat("u_time", curr_time * 0.5);

  glDrawArrays(GL_TRIANGLES, 0, 36);
  glDepthFunc(GL_LESS);

  DrawQuads();

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  ShaderManager::Get().GetShader("quad")->Bind();
  full_screen_quad_vao_.Bind();
  glDisable(GL_DEPTH_TEST);
  glBindTexture(GL_TEXTURE_2D, fbo1_tex_);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void Renderer::SetDrawIndirectBufferData(
    Buffer& draw_indirect_buffer, std::vector<uint32_t>& frame_draw_cmd_mesh_ids,
    std::vector<DrawElementsIndirectCommand>& frame_dei_cmds,
    std::unordered_map<uint32_t, DrawElementsIndirectCommand>& dei_cmds) {
  ZoneScoped;
  {
    ZoneScopedN("Clear per frame and reserve");
    frame_dei_cmds.clear();
    frame_dei_cmds.reserve(frame_draw_cmd_mesh_ids.size());
  }
  DrawElementsIndirectCommand cmd;
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
}

void Renderer::SubmitChunkDrawCommand(const glm::mat4& model, uint32_t mesh_handle) {
  ZoneScoped;
  chunk_frame_uniforms_mesh_ids_.second.emplace_back(mesh_handle);
  chunk_frame_uniforms_mesh_ids_.first.emplace_back(model);
}

void Renderer::SubmitRegMeshDrawCommand(const glm::mat4& model, uint32_t mesh_handle,
                                        uint32_t material_handle) {
  ZoneScoped;
  reg_mesh_frame_uniforms_mesh_ids_.first.emplace_back(model, material_allocs_[material_handle]);
  reg_mesh_frame_uniforms_mesh_ids_.second.emplace_back(mesh_handle);
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

uint32_t Renderer::AllocateChunk(std::vector<ChunkVertex>& vertices,
                                 std::vector<uint32_t>& indices) {
  ZoneScoped;
  if (vertices.empty() || indices.empty()) {
    return 0;
  }
  uint32_t chunk_vbo_offset;
  uint32_t chunk_ebo_offset;
  uint32_t id = rand();
  uint32_t vbo_handle;
  uint32_t ebo_handle;
  ebo_handle =
      chunk_ebo_.Allocate(sizeof(uint32_t) * indices.size(), indices.data(), chunk_ebo_offset);

  vbo_handle =
      chunk_vbo_.Allocate(sizeof(ChunkVertex) * vertices.size(), vertices.data(), chunk_vbo_offset);
  stats_.total_chunk_vertices += vertices.size();
  stats_.total_chunk_indices += indices.size();
  chunk_allocs_.emplace(id, MeshAlloc{
                                .vbo_handle = vbo_handle,
                                .ebo_handle = ebo_handle,
                                .vertices_count = static_cast<uint32_t>(vertices.size()),
                                .indices_count = static_cast<uint32_t>(indices.size()),
                            });
  chunk_dei_cmds_.emplace(
      id, DrawElementsIndirectCommand{
              .count = static_cast<uint32_t>(indices.size()),
              .instance_count = 0,
              .first_index = static_cast<uint32_t>(chunk_ebo_offset / sizeof(uint32_t)),
              .base_vertex = static_cast<uint32_t>(chunk_vbo_offset / sizeof(ChunkVertex)),
              .base_instance = 0,
          });
  return id;
}
uint32_t Renderer::AllocateStaticChunk(std::vector<ChunkVertex>& vertices,
                                       std::vector<uint32_t>& indices, const glm::ivec3& pos,
                                       LODLevel level) {
  ZoneScoped;
  if (vertices.empty() || indices.empty()) {
    spdlog::error("no vertices or indices");
    return 0;
  }
  uint32_t chunk_vbo_offset;
  uint32_t chunk_ebo_offset;
  uint32_t id = next_static_chunk_handle_++;
  id = (static_cast<int>(level) << 29 | id);
  uint32_t vbo_handle;
  uint32_t ebo_handle;
  glm::ivec4 min = glm::ivec4(pos, 0);
  glm::ivec4 max = min + ChunkLength;
  if (level == LODLevel::Regular) {
    ebo_handle = static_chunk_ebo_.Allocate(sizeof(uint32_t) * indices.size(), indices.data(),
                                            chunk_ebo_offset);
    vbo_handle = static_chunk_vbo_.Allocate(
        sizeof(ChunkVertex) * vertices.size(), vertices.data(), chunk_vbo_offset,
        {
            .aabb = {min, max},
            .first_index = static_cast<uint32_t>(chunk_ebo_offset / sizeof(uint32_t)),
            .count = static_cast<uint32_t>(indices.size()),
        });
    static_chunk_allocs_.try_emplace(id,
                                     MeshAlloc{
                                         .vbo_handle = vbo_handle,
                                         .ebo_handle = ebo_handle,
                                         .vertices_count = static_cast<uint32_t>(vertices.size()),
                                         .indices_count = static_cast<uint32_t>(indices.size()),
                                     });
    stats_.total_chunk_vertices += vertices.size();
    stats_.total_chunk_indices += indices.size();
    static_chunk_buffer_dirty_ = true;
  } else {
    max.y = MaxBlockHeight;
    ebo_handle = lod_static_chunk_ebo_.Allocate(sizeof(uint32_t) * indices.size(), indices.data(),
                                                chunk_ebo_offset);
    vbo_handle = lod_static_chunk_vbo_.Allocate(
        sizeof(ChunkVertex) * vertices.size(), vertices.data(), chunk_vbo_offset,
        {
            .aabb = {min, max},
            .first_index = static_cast<uint32_t>(chunk_ebo_offset / sizeof(uint32_t)),
            .count = static_cast<uint32_t>(indices.size()),
        });
    lod_static_chunk_allocs_.try_emplace(
        id, MeshAlloc{
                .vbo_handle = vbo_handle,
                .ebo_handle = ebo_handle,
                .vertices_count = static_cast<uint32_t>(vertices.size()),
                .indices_count = static_cast<uint32_t>(indices.size()),
            });
    stats_.total_chunk_vertices += vertices.size();
    stats_.total_chunk_indices += indices.size();
    stats_.lod_chunk_vertices += vertices.size();
    stats_.lod_chunk_indices += indices.size();
    lod_static_chunk_buffer_dirty_ = true;
  }
  return id;
}

uint32_t Renderer::AllocateMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
  ZoneScoped;
  if (vertices.empty() || indices.empty()) {
    spdlog::error("no vertices or indices");
    return 0;
  }
  uint32_t vbo_offset;
  uint32_t ebo_offset;
  uint32_t id = rand();
  // TODO: handle error case of no space/failed alloc
  reg_mesh_allocs_.try_emplace(
      id, MeshAlloc{.vbo_handle = reg_mesh_vbo_.Allocate(sizeof(Vertex) * vertices.size(),
                                                         vertices.data(), vbo_offset),
                    .ebo_handle = reg_mesh_ebo_.Allocate(sizeof(uint32_t) * indices.size(),
                                                         indices.data(), ebo_offset),
                    .vertices_count = static_cast<uint32_t>(vertices.size()),
                    .indices_count = static_cast<uint32_t>(indices.size())});
  reg_mesh_dei_cmds_.try_emplace(
      id, DrawElementsIndirectCommand{
              .count = static_cast<uint32_t>(indices.size()),
              .instance_count = 0,
              .first_index = static_cast<uint32_t>(ebo_offset / sizeof(uint32_t)),
              .base_vertex = static_cast<uint32_t>(vbo_offset / sizeof(Vertex)),
              .base_instance = 0,
          });
  stats_.total_reg_mesh_vertices += vertices.size();
  stats_.total_reg_mesh_indices += indices.size();
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
  stats_.total_reg_mesh_vertices -= it->second.vertices_count;
  stats_.total_reg_mesh_indices -= it->second.indices_count;
  reg_mesh_vbo_.Free(it->second.vbo_handle);
  reg_mesh_ebo_.Free(it->second.ebo_handle);
  reg_mesh_dei_cmds_.erase(it->first);
  reg_mesh_allocs_.erase(it);
}

void Renderer::FreeChunkMesh(uint32_t handle) {
  if (handle == 0) return;
  auto it = chunk_allocs_.find(handle);
  if (it == chunk_allocs_.end()) {
    spdlog::error("FreeChunkMesh: handle not found: {}", handle);
  }
  stats_.total_chunk_indices -= it->second.indices_count;
  stats_.total_chunk_vertices -= it->second.vertices_count;
  chunk_vbo_.Free(it->second.vbo_handle);
  chunk_ebo_.Free(it->second.ebo_handle);
  chunk_dei_cmds_.erase(it->first);
  chunk_allocs_.erase(it);
}

void Renderer::FreeStaticChunkMesh(uint32_t handle) {
  if (handle == 0) return;
  auto level = static_cast<LODLevel>(handle >> 29);
  if (level == LODLevel::Regular) {
    static_chunk_buffer_dirty_ = true;
    auto it = static_chunk_allocs_.find(handle);
    if (it == static_chunk_allocs_.end()) {
      spdlog::error("FreeStaticChunkMesh: Regular handle not found: {}", handle);
      return;
    }
    stats_.total_chunk_indices -= it->second.indices_count;
    stats_.total_chunk_vertices -= it->second.vertices_count;
    static_chunk_vbo_.Free(it->second.vbo_handle);
    static_chunk_ebo_.Free(it->second.ebo_handle);
    static_chunk_allocs_.erase(it);
  } else {
    lod_static_chunk_buffer_dirty_ = true;
    auto it = lod_static_chunk_allocs_.find(handle);
    if (it == lod_static_chunk_allocs_.end()) {
      spdlog::error("FreeStaticChunkMesh: LOD handle not found: {}", handle);
      return;
    }
    stats_.total_chunk_indices -= it->second.indices_count;
    stats_.total_chunk_vertices -= it->second.vertices_count;
    stats_.lod_chunk_indices -= it->second.indices_count;
    stats_.lod_chunk_indices -= it->second.vertices_count;
    lod_static_chunk_vbo_.Free(it->second.vbo_handle);
    lod_static_chunk_ebo_.Free(it->second.ebo_handle);
    lod_static_chunk_allocs_.erase(it);
  }
}

void Renderer::RemoveStaticMeshes() {
  static_textured_quad_uniforms_.clear();
  stats_.textured_quad_draw_calls = 0;
}

void Renderer::StartFrame(const Window&) {
  {
    ZoneScopedN("clear buffers");
    // TODO: don't clear and reallocate, or at least profile in the future
    reg_mesh_frame_uniforms_mesh_ids_.first.clear();
    reg_mesh_frame_uniforms_mesh_ids_.second.clear();
    lines_frame_uniforms_mesh_ids_.first.clear();
    lines_frame_uniforms_mesh_ids_.second.clear();
    lines_no_depth_frame_uniforms_mesh_ids_.first.clear();
    lines_no_depth_frame_uniforms_mesh_ids_.second.clear();
    chunk_frame_uniforms_mesh_ids_.first.clear();
    chunk_frame_uniforms_mesh_ids_.second.clear();
    quad_textured_uniforms_.clear();
    quad_color_uniforms_.clear();
    stats_.textured_quad_draw_calls = 0;
    stats_.color_quad_draw_calls = 0;
  }

  { ZoneScopedN("OpenGL state"); }
}

bool Renderer::OnEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_r && event.key.keysym.mod & KMOD_ALT) {
        ShaderManager::Get().RecompileShaders();
        return true;
      }
      if (event.key.keysym.sym == SDLK_w && event.key.keysym.mod & KMOD_ALT) {
        wireframe_enabled_ = !wireframe_enabled_;
        return true;
      }
    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
        HandleResize(event.window.data1, event.window.data2);
        return true;
      }
  }
  return false;
}

void Renderer::LoadShaders() {
  ShaderManager::Get().AddShader("quad", {{GET_SHADER_PATH("quad.vs.glsl"), ShaderType::Vertex},
                                          {GET_SHADER_PATH("quad.fs.glsl"), ShaderType::Fragment}});
  ShaderManager::Get().AddShader("skybox",
                                 {{GET_SHADER_PATH("skybox.vs.glsl"), ShaderType::Vertex},
                                  {GET_SHADER_PATH("skybox.fs.glsl"), ShaderType::Fragment}});
  ShaderManager::Get().AddShader("sky", {{GET_SHADER_PATH("sky.vs.glsl"), ShaderType::Vertex},
                                         {GET_SHADER_PATH("sky.fs.glsl"), ShaderType::Fragment}});
  ShaderManager::Get().AddShader("color",
                                 {{GET_SHADER_PATH("color.vs.glsl"), ShaderType::Vertex},
                                  {GET_SHADER_PATH("color.fs.glsl"), ShaderType::Fragment}});
  ShaderManager::Get().AddShader("color_single",
                                 {{GET_SHADER_PATH("color_single.vs.glsl"), ShaderType::Vertex},
                                  {GET_SHADER_PATH("color_single.fs.glsl"), ShaderType::Fragment}});
  ShaderManager::Get().AddShader(
      "chunk_batch_block", {{GET_SHADER_PATH("chunk_batch_block.vs.glsl"), ShaderType::Vertex},
                            {GET_SHADER_PATH("chunk_batch_block.fs.glsl"), ShaderType::Fragment}});
  ShaderManager::Get().AddShader("chunk_batch",
                                 {{GET_SHADER_PATH("chunk_batch.vs.glsl"), ShaderType::Vertex},
                                  {GET_SHADER_PATH("chunk_batch.fs.glsl"), ShaderType::Fragment}});
  ShaderManager::Get().AddShader(
      "lod_chunk_batch", {{GET_SHADER_PATH("lod_chunk_batch.vs.glsl"), ShaderType::Vertex},
                          {GET_SHADER_PATH("lod_chunk_batch.fs.glsl"), ShaderType::Fragment}});
  ShaderManager::Get().AddShader(
      "single_texture", {{GET_SHADER_PATH("single_texture.vs.glsl"), ShaderType::Vertex},
                         {GET_SHADER_PATH("single_texture.fs.glsl"), ShaderType::Fragment}});
  ShaderManager::Get().AddShader(
      "block_outline", {{GET_SHADER_PATH("block_outline.vs.glsl"), ShaderType::Vertex},
                        {GET_SHADER_PATH("block_outline.gs.glsl"), ShaderType::Geometry},
                        {GET_SHADER_PATH("block_outline.fs.glsl"), ShaderType::Fragment}});
  ShaderManager::Get().AddShader("chunk_cull",
                                 {{GET_SHADER_PATH("chunk_cull.cs.glsl"), ShaderType::Compute}});
}

void Renderer::OnImGui() {
  ImGui::Begin("Renderer");
  ImGui::Text("Chunk Vertices: %i", stats_.total_chunk_vertices);
  ImGui::Text("Chunk Indices: %i", stats_.total_chunk_indices);
  ImGui::Text("Static Chunk VBO Allocs: %i", static_chunk_vbo_.NumActiveAllocs());
  ImGui::Text("Static Chunk EBO Allocs: %i", static_chunk_ebo_.NumActiveAllocs());
  ImGui::Checkbox("Cull Frustum", &settings.cull_frustum);
  ImGui::Checkbox("Chunk Use Texture", &settings.chunk_render_use_texture);
  ImGui::Checkbox("Chunk Use AO", &settings.chunk_use_ao);
  ImGui::SliderInt("Extra FOV Degrees", &settings.extra_fov_degrees, 0, 360);
  ImGui::SliderFloat("Chunk Cull Distance Min", &settings.chunk_cull_distance_min, 0, 10000);
  ImGui::SliderFloat("Chunk Cull Distance Max", &settings.chunk_cull_distance_max, 0, 10000);
  ImGui::End();
}

void Renderer::HandleResize(int new_width, int new_height) {
  glViewport(0, 0, new_width, new_height);
  InitFrameBuffers();
}
