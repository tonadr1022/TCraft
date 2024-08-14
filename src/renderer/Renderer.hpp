#pragma once

#include <SDL_events.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include "renderer/ChunkRenderParams.hpp"
#include "renderer/Common.hpp"
#include "renderer/Mesh.hpp"
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

class Window;
class WorldScene;
class BlockEditorScene;
class BlockDB;
struct Vertex;
struct ChunkVertex;
struct TextureMaterialData;

struct RenderInfo {
  glm::mat4 vp_matrix;
  glm::mat4 view_matrix;
  glm::mat4 proj_matrix;
  glm::vec3 view_pos;
};

class Renderer {
 public:
  static Renderer& Get();
  ~Renderer();
  static void Init(Window& window);
  static void Shutdown();
  void OnImGui();

  bool render_gui_{true};
  bool render_chunks_{true};
  bool wireframe_enabled_{false};

  void RenderWorld(const ChunkRenderParams& render_params, const RenderInfo& render_info);
  // void Reset();
  void SubmitChunkDrawCommand(const glm::mat4& model, uint32_t mesh_handle);
  void SubmitRegMeshDrawCommand(const glm::mat4& model, uint32_t mesh_handle,
                                uint32_t material_handle);
  [[nodiscard]] uint32_t AllocateStaticChunk(std::vector<ChunkVertex>& vertices,
                                             std::vector<uint32_t>& indices, const glm::ivec3& pos);
  [[nodiscard]] uint32_t AllocateChunk(std::vector<ChunkVertex>& vertices,
                                       std::vector<uint32_t>& indices);

  [[nodiscard]] uint32_t AllocateMesh(std::vector<Vertex>& vertices,
                                      std::vector<uint32_t>& indices);
  void DrawQuad(uint32_t material_handle, const glm::vec2& center, const glm::vec2& size);
  void DrawQuad(const glm::vec3& color, const glm::vec2& center, const glm::vec2& size);
  void AddStaticQuad(uint32_t material_handle, const glm::vec2& center, const glm::vec2& size);
  void AddStaticQuads(std::vector<UserDrawCommand>& static_quad_draw_cmds);
  void RemoveStaticMeshes();

  void FreeStaticChunkMesh(uint32_t handle);
  void FreeChunkMesh(uint32_t handle);
  void FreeRegMesh(uint32_t handle);
  [[nodiscard]] uint32_t AllocateMaterial(TextureMaterialData& material);
  [[nodiscard]] uint32_t AllocateTextureMaterial(uint32_t texture_handle);
  void FreeMaterial(uint32_t material_handle);

  void Init();
  void StartFrame(const Window& window);
  [[nodiscard]] bool OnEvent(const SDL_Event& event);
  void DrawBlockOutline(const glm::vec3& block_pos, const glm::mat4& view,
                        const glm::mat4& projection);

  void RenderQuads();
  void RenderRegMeshes();

 private:
  friend class Application;
  static Renderer* instance_;
  explicit Renderer(Window& window);
  Window& window_;

  constexpr const static uint32_t MaxDrawCmds{1'000'000};
  constexpr const static uint32_t MaxChunkDrawCmds{1'000'00};
  constexpr const static uint32_t MaxUIDrawCmds{10'000};
  ShaderManager shader_manager_;

  // TODO: try without alignas
  struct StaticChunkDrawCmdUniform {
    glm::vec4 pos;
  };

  struct ChunkDrawCmdUniform {
    glm::mat4 model;
  };

  // TODO: try without alignas
  struct alignas(16) MaterialUniforms {
    glm::mat4 model;
    uint64_t material_index;
  };

  struct alignas(16) ColorUniforms {
    glm::mat4 model;
    glm::vec3 color;
  };

  struct MeshAlloc {
    uint32_t vbo_handle;
    uint32_t ebo_handle;
  };

  struct ChunkMeshAlloc {
    glm::ivec3 pos;
    uint32_t vbo_handle;
    uint32_t ebo_handle;
  };

  struct alignas(16) UBOUniforms {
    glm::mat4 vp_matrix;
    glm::vec3 cam_pos;
    float pad;
  };
  Buffer uniform_ubo_;

  bool cull_frustum_{true};

  VertexArray static_chunk_vao_;
  DynamicBuffer<> static_chunk_ebo_;
  VertexArray chunk_vao_;
  DynamicBuffer<> chunk_ebo_;

  struct AABB {
    glm::vec4 min;
    glm::vec4 max;
  };

  // Needs to be 16 byte aligned for the GPU
  struct alignas(16) ChunkDrawInfo {
    AABB aabb;
    uint32_t first_index;
    uint32_t count;
  };
  void RenderStaticChunks(const ChunkRenderParams& render_params, const RenderInfo& render_info);

  DynamicBuffer<> chunk_vbo_;

  DynamicBuffer<ChunkDrawInfo> static_chunk_vbo_;
  Buffer static_chunk_draw_info_buffer_;
  Buffer static_chunk_draw_count_buffer_;
  Buffer static_chunk_uniform_ssbo_;
  Buffer static_chunk_draw_indirect_buffer_;
  Buffer chunk_draw_indirect_buffer_;
  Buffer chunk_uniform_ssbo_;
  std::unordered_map<uint32_t, MeshAlloc> static_chunk_allocs_;
  std::unordered_map<uint32_t, MeshAlloc> chunk_allocs_;
  std::unordered_map<uint32_t, DrawElementsIndirectCommand> chunk_dei_cmds_;
  std::vector<uint32_t> chunk_frame_draw_cmd_mesh_ids_;
  std::vector<ChunkDrawCmdUniform> chunk_frame_draw_cmd_uniforms_;
  std::vector<DrawElementsIndirectCommand> chunk_frame_dei_cmds_;
  bool static_chunk_buffer_dirty_{true};

  VertexArray reg_mesh_vao_;
  DynamicBuffer<> reg_mesh_vbo_;
  DynamicBuffer<> reg_mesh_ebo_;
  Buffer reg_mesh_uniform_ssbo_;
  Buffer reg_mesh_draw_indirect_buffer_;
  std::unordered_map<uint32_t, MeshAlloc> reg_mesh_allocs_;
  std::unordered_map<uint32_t, DrawElementsIndirectCommand> reg_mesh_dei_cmds_;
  std::vector<uint32_t> reg_mesh_frame_draw_cmd_mesh_ids_;
  std::vector<MaterialUniforms> reg_mesh_frame_draw_cmd_uniforms_;
  std::vector<DrawElementsIndirectCommand> reg_mesh_frame_dei_cmds_;

  DynamicBuffer<> tex_materials_buffer_;
  std::unordered_map<uint32_t, uint32_t> material_allocs_;

  // std::vector<DrawCmdUniform> quad_frame_draw_cmd_uniforms_;
  VertexArray quad_vao_;
  Buffer quad_vbo_;
  Buffer quad_ebo_;
  Buffer textured_quad_uniform_ssbo_;
  std::vector<MaterialUniforms> quad_textured_uniforms_;
  Buffer quad_draw_indirect_buffer_;
  std::vector<MaterialUniforms> static_textured_quad_uniforms_;

  Buffer quad_color_uniform_ssbo_;
  std::vector<ColorUniforms> quad_color_uniforms_;

  VertexArray cube_vao_;
  Buffer cube_vbo_;
  Buffer cube_ebo_;

  struct Stats {
    uint32_t textured_quad_draw_calls{0};
    uint32_t color_quad_draw_calls{0};
  };
  Stats stats_;

  void LoadShaders();

  template <typename UniformType>
  void SetMeshFrameDrawCommands(
      Buffer& draw_indirect_buffer, std::vector<UniformType>& frame_draw_cmd_uniforms,
      std::vector<uint32_t>& frame_draw_cmd_mesh_ids,
      std::vector<DrawElementsIndirectCommand>& frame_dei_cmds,
      std::unordered_map<uint32_t, DrawElementsIndirectCommand>& dei_cmds);
  void ShutdownInternal();
};
