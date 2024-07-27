#include "Renderer.hpp"

#include <cstdint>

#include "ShaderManager.hpp"
#include "application/SettingsManager.hpp"
#include "application/Window.hpp"
#include "gameplay/scene/BlockEditorScene.hpp"
#include "gameplay/scene/WorldScene.hpp"
#include "pch.hpp"
#include "renderer/ChunkMesh.hpp"
#include "renderer/opengl/Debug.hpp"
#include "renderer/opengl/Texture2dArray.hpp"
#include "resource/TextureManager.hpp"
#include "util/Paths.hpp"

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
  chunk_vbo_.Init(sizeof(ChunkVertex) * 10'000'000, GL_DYNAMIC_STORAGE_BIT);
  chunk_ebo_.Init(sizeof(uint32_t) * 10'000'000, GL_DYNAMIC_STORAGE_BIT);
  chunk_vao_.AttachVertexBuffer(chunk_vbo_.Id(), 0, 0, sizeof(ChunkVertex));
  chunk_vao_.AttachElementBuffer(chunk_ebo_.Id());
  chunk_uniform_ssbo_.Init(sizeof(ChunkDrawCmdUniform) * MaxDrawCmds, GL_DYNAMIC_STORAGE_BIT);
  chunk_draw_indirect_buffer_.Init(sizeof(DrawElementsIndirectCommand) * MaxDrawCmds,
                                   GL_DYNAMIC_STORAGE_BIT);
}

void Renderer::Shutdown() {
  nlohmann::json settings;
  settings["wireframe_enabled"] = wireframe_enabled_;
  SettingsManager::Get().SaveSetting(settings, "renderer");
}

void Renderer::Reset() {
  spdlog::info("resetting renderer buffers");
  chunk_ebo_.ResetOffset();
  chunk_vbo_.ResetOffset();
}

void Renderer::RenderBlockEditor(const BlockEditorScene& scene, const RenderInfo& render_info) {
  ZoneScoped;
  SetFrameDrawCommands();

  auto shader = shader_manager_.GetShader("chunk_batch");
  shader->Bind();
  shader->SetMat4("vp_matrix", render_info.vp_matrix);
  const auto& chunk_tex_arr =
      TextureManager::Get().GetTexture2dArray(scene.render_params_.chunk_tex_array_handle);
  chunk_tex_arr.Bind(0);
  chunk_vao_.Bind();
  glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, frame_dei_cmds_.size(), 0);
}

void Renderer::RenderWorld(const WorldScene& world, const RenderInfo& render_info) {
  ZoneScoped;
  {
    ZoneScopedN("Chunk render");

    {
      ZoneScopedN("Submit chunk draw commands");
      // TODO: only send to renderer the chunks ready to be rendered instead of the whole map
      for (const auto& it : world.chunk_manager_.GetVisibleChunks()) {
        auto& mesh = it.second->GetMesh();
        glm::vec3 pos = it.first * ChunkLength;
        SubmitChunkDrawCommand(glm::translate(glm::mat4{1}, pos), mesh.handle_);
      }
    }

    SetFrameDrawCommands();

    {
      ZoneScopedN("Render chunks");
      auto shader = shader_manager_.GetShader("chunk_batch");
      shader->Bind();
      shader->SetMat4("vp_matrix", render_info.vp_matrix);

      const auto& chunk_tex_arr = TextureManager::Get().GetTexture2dArray(
          world.world_render_params_.chunk_tex_array_handle);
      chunk_tex_arr.Bind(0);
      chunk_vao_.Bind();
      glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, frame_dei_cmds_.size(),
                                  0);
    }
  }
}

void Renderer::SetFrameDrawCommands() {
  ZoneScoped;
  chunk_uniform_ssbo_.ResetOffset();
  chunk_uniform_ssbo_.SubData(sizeof(ChunkDrawCmdUniform) * frame_chunk_draw_cmd_uniforms_.size(),
                              frame_chunk_draw_cmd_uniforms_.data());
  {
    ZoneScopedN("Clear per frame and reserve");
    frame_dei_cmds_.clear();
    frame_dei_cmds_.reserve(frame_chunk_draw_cmd_uniforms_.size());
  }
  DrawElementsIndirectCommand cmd;
  uint32_t base_instance{0};
  EASSERT_MSG(frame_chunk_draw_cmd_uniforms_.size() == frame_draw_cmd_mesh_ids_.size(),
              "Per frame draw cmd size must equal mesh cmd size");
  for (uint32_t i = 0; i < frame_draw_cmd_mesh_ids_.size(); i++) {
    const auto& draw_cmd_info = dei_cmds_[frame_draw_cmd_mesh_ids_[i]];
    cmd.base_instance = i;
    cmd.base_vertex = draw_cmd_info.base_vertex;
    cmd.instance_count = 1;
    cmd.count = draw_cmd_info.count;
    cmd.first_index = draw_cmd_info.first_index;
    frame_dei_cmds_.emplace_back(cmd);
  }

  chunk_draw_indirect_buffer_.ResetOffset();
  chunk_draw_indirect_buffer_.SubData(sizeof(DrawElementsIndirectCommand) * frame_dei_cmds_.size(),
                                      frame_dei_cmds_.data());
  chunk_draw_indirect_buffer_.Bind(GL_DRAW_INDIRECT_BUFFER);
  chunk_uniform_ssbo_.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
}

void Renderer::SubmitChunkDrawCommand(const glm::mat4& model, uint32_t mesh_handle) {
  frame_draw_cmd_mesh_ids_.emplace_back(mesh_handle);
  frame_chunk_draw_cmd_uniforms_.emplace_back(model);
}

uint32_t Renderer::AllocateChunk(std::vector<ChunkVertex>& vertices,
                                 std::vector<uint32_t>& indices) {
  uint32_t id = dei_cmds_.size();
  dei_cmds_.emplace_back(DrawElementsIndirectCommand{
      .count = static_cast<uint32_t>(indices.size()),
      .instance_count = 0,
      .first_index = static_cast<uint32_t>(chunk_ebo_.Offset() / sizeof(uint32_t)),
      .base_vertex = static_cast<uint32_t>(chunk_vbo_.Offset() / sizeof(ChunkVertex)),
      .base_instance = 0,
  });
  chunk_vbo_.SubData(sizeof(ChunkVertex) * vertices.size(), vertices.data());
  chunk_ebo_.SubData(sizeof(uint32_t) * indices.size(), indices.data());
  return id;
}

void Renderer::StartFrame(const Window& window) {
  // TODO: don't clear and reallocate, or at least profile in the future
  frame_draw_cmd_mesh_ids_.clear();
  frame_chunk_draw_cmd_uniforms_.clear();
  auto dims = window.GetWindowSize();
  glViewport(0, 0, dims.x, dims.y);
  glClearColor(1, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPolygonMode(GL_FRONT_AND_BACK, wireframe_enabled_ ? GL_LINE : GL_FILL);
}

void Renderer::Render(const Window& window) const {}

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
  // shader_manager_.AddShader("chunk", {{GET_SHADER_PATH("chunk.vs.glsl"), ShaderType::Vertex},
  //                                     {GET_SHADER_PATH("chunk.fs.glsl"), ShaderType::Fragment}});
  shader_manager_.AddShader("chunk_batch",
                            {{GET_SHADER_PATH("chunk_batch.vs.glsl"), ShaderType::Vertex},
                             {GET_SHADER_PATH("chunk_batch.fs.glsl"), ShaderType::Fragment}});
}

Renderer::~Renderer() = default;
