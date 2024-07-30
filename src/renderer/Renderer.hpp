#pragma once

#include <SDL_events.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include "renderer/ChunkRenderParams.hpp"
#include "renderer/ShaderManager.hpp"
#include "renderer/opengl/Buffer.hpp"
#include "renderer/opengl/DynamicBuffer.hpp"
#include "renderer/opengl/VertexArray.hpp"

struct DrawElementsIndirectCommand {
  uint32_t count;
  uint32_t instance_count;
  uint32_t first_index;
  uint32_t base_vertex;
  uint32_t base_instance;
};

class WorldScene;
class BlockEditorScene;
class Window;
class BlockDB;
struct Vertex;
struct ChunkVertex;

struct RenderInfo {
  glm::mat4 vp_matrix;
};

class Renderer {
 public:
  static Renderer& Get();
  bool render_gui_{true};
  bool render_chunks_{true};
  bool wireframe_enabled_{false};

  void RenderWorld(const ChunkRenderParams& render_params, const RenderInfo& render_info);
  // void Reset();
  void SubmitChunkDrawCommand(const glm::mat4& model, uint32_t mesh_handle);
  [[nodiscard]] uint32_t AllocateMesh(std::vector<ChunkVertex>& vertices,
                                      std::vector<uint32_t>& indices);

  // [[nodiscard]] uint32_t AllocateMesh(std::vector<Vertex>& vertices,
  //                                     std::vector<uint32_t>& indices);

  void FreeChunkMesh(uint32_t handle);
  void FreeMesh(uint32_t handle);

  void Init();
  void StartFrame(const Window& window);
  bool OnEvent(const SDL_Event& event);
  void Shutdown();

 private:
  friend class Application;
  static Renderer* instance_;
  Renderer();

  constexpr const static uint32_t MaxDrawCmds{1'000'000};
  ShaderManager shader_manager_;

  struct ChunkDrawCmdUniform {
    glm::mat4 model;
  };

  VertexArray chunk_vao_;
  DynamicBuffer chunk_vbo_;
  DynamicBuffer chunk_ebo_;
  Buffer chunk_uniform_ssbo_;
  Buffer chunk_draw_indirect_buffer_;

  VertexArray default_vao_;
  DynamicBuffer default_vbo_;
  DynamicBuffer default_ebo_;
  Buffer default_uniform_ssbo_;
  Buffer default_draw_indirect_buffer_;

  struct ChunkAlloc {
    uint32_t vbo_handle;
    uint32_t ebo_handle;
  };

  std::unordered_map<uint32_t, ChunkAlloc> chunk_allocs_;
  std::unordered_map<uint32_t, DrawElementsIndirectCommand> dei_cmds_;

  std::vector<uint32_t> frame_draw_cmd_mesh_ids_;
  std::vector<ChunkDrawCmdUniform> frame_chunk_draw_cmd_uniforms_;
  std::vector<DrawElementsIndirectCommand> frame_dei_cmds_;
  // std::vector<DrawElementsIndirectCommand> dei_cmds_;

  void LoadShaders();
  void SetChunkFrameDrawCommands();
};
