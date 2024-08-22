#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include "gameplay/world/ChunkDef.hpp"
#include "renderer/Common.hpp"
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
struct RenderInfo;
class Texture;
union SDL_Event;

class Renderer {
 public:
  static Renderer& Get();
  ~Renderer();
  static void Init(Window& window);
  static void Shutdown();
  void OnImGui();

  bool render_gui{true};
  bool render_chunks_{true};
  bool wireframe_enabled_{false};
  std::shared_ptr<Texture> chunk_tex_array{nullptr};

  void SetUBOData(const RenderInfo& render_info);
  void Render(const RenderInfo& render_info);
  void BeginRender() const;
  // void Reset();
  void SubmitChunkDrawCommand(const glm::mat4& model, uint32_t mesh_handle);
  void SubmitRegMeshDrawCommand(const glm::mat4& model, uint32_t mesh_handle,
                                uint32_t material_handle);
  [[nodiscard]] uint32_t AllocateStaticChunk(std::vector<ChunkVertex>& vertices,
                                             std::vector<uint32_t>& indices, const glm::ivec3& pos,
                                             LODLevel level);
  uint32_t next_static_chunk_handle_{1};

  [[nodiscard]] uint32_t AllocateChunk(std::vector<ChunkVertex>& vertices,
                                       std::vector<uint32_t>& indices);

  [[nodiscard]] uint32_t AllocateMesh(std::vector<Vertex>& vertices,
                                      std::vector<uint32_t>& indices);
  void DrawQuad(uint32_t material_handle, const glm::vec2& center, const glm::vec2& size);
  void DrawQuad(uint32_t material_handle, const glm::vec3& pos, const glm::vec2& size);

  void DrawQuad(const glm::vec3& color, const glm::vec2& center, const glm::vec2& size);
  void AddStaticQuad(uint32_t material_handle, const glm::vec2& center, const glm::vec2& size);
  void AddStaticQuads(std::vector<UserDrawCommand>& static_quad_draw_cmds);
  void RemoveStaticMeshes();

  uint32_t fbo1_{};
  uint32_t fbo1_tex_{};
  uint32_t fbo1_depth_tex_{};
  uint32_t rbo1_{};

  void FreeStaticChunkMesh(uint32_t handle);
  void FreeChunkMesh(uint32_t handle);
  void FreeRegMesh(uint32_t handle);
  [[nodiscard]] uint32_t AllocateMaterial(TextureMaterialData& material);
  void FreeMaterial(uint32_t& material_handle);

  void Init();
  void StartFrame(const Window& window);
  [[nodiscard]] bool OnEvent(const SDL_Event& event);
  void DrawLine(const glm::mat4& model, const glm::vec3& color, uint32_t mesh_handle,
                bool ignore_depth = false);

  struct Settings {
    float chunk_cull_distance_min{0};
    float chunk_cull_distance_max{10000};
    int extra_fov_degrees{0};
    bool cull_frustum{true};
    bool chunk_render_use_texture{true};
    bool chunk_use_ao{true};
    bool draw_lines{true};
    bool draw_skybox{true};
    bool draw_chunks{true};
    bool draw_regular_meshes{true};
    bool draw_quads{true};
  };
  Settings settings;

  // The passed callback function should bind a shader and set any uniforms necessary, meant for a
  // procedural skybox rather than a static image.
  void SetSkyboxShaderFunc(const std::function<void()>& func);

 private:
  static Renderer* instance_;
  explicit Renderer(Window& window);
  Window& window_;

  constexpr const static uint32_t MaxDrawCmds{1'000'000};
  constexpr const static uint32_t MaxChunkDrawCmds{1'000'00};
  constexpr const static uint32_t MaxUIDrawCmds{10'000};

  void DrawQuads();
  void RenderRegMeshes();
  void DrawStaticChunks(const RenderInfo& render_info);
  void DrawNonStaticChunks(const RenderInfo& render_info);
  void DrawRegularMeshes(const RenderInfo& render_info);
  void DrawLines(const RenderInfo& render_info);
  // std::vector<OutlineDrawCmd> outline_draw_cmds_;

  // TODO: try without alignas
  struct DrawCmdUniformPosOnly {
    glm::vec4 pos;
  };

  struct DrawCmdUniformModelOnly {
    glm::mat4 model;
  };

  // TODO: try without alignas
  struct alignas(16) UniformsModelMaterial {
    glm::mat4 model;
    uint64_t material_index;
  };

  struct alignas(16) UniformsModelColor {
    glm::mat4 model;
    glm::vec3 color;
  };

  struct MeshAlloc {
    uint32_t vbo_handle;
    uint32_t ebo_handle;
    uint32_t vertices_count;
    uint32_t indices_count;
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

  VertexArray static_chunk_vao_;
  DynamicBuffer<> static_chunk_ebo_;
  VertexArray chunk_vao_;
  DynamicBuffer<> chunk_ebo_;
  VertexArray lod_static_chunk_vao_;
  DynamicBuffer<> lod_static_chunk_ebo_;

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

  DynamicBuffer<> chunk_vbo_;

  DynamicBuffer<ChunkDrawInfo> static_chunk_vbo_;
  Buffer static_chunk_draw_info_buffer_;
  Buffer static_chunk_draw_count_buffer_;
  Buffer static_chunk_uniform_ssbo_;
  Buffer static_chunk_draw_indirect_buffer_;
  Buffer chunk_draw_indirect_buffer_;
  Buffer chunk_uniform_ssbo_;
  std::unordered_map<uint32_t, MeshAlloc> static_chunk_allocs_;

  DynamicBuffer<ChunkDrawInfo> lod_static_chunk_vbo_;
  Buffer lod_static_chunk_draw_info_buffer_;
  Buffer lod_static_chunk_draw_count_buffer_;
  Buffer lod_static_chunk_uniform_ssbo_;
  Buffer lod_static_chunk_draw_indirect_buffer_;
  Buffer lod_chunk_draw_indirect_buffer_;
  Buffer lod_chunk_uniform_ssbo_;
  std::unordered_map<uint32_t, MeshAlloc> lod_static_chunk_allocs_;

  std::unordered_map<uint32_t, MeshAlloc> chunk_allocs_;
  std::unordered_map<uint32_t, DrawElementsIndirectCommand> chunk_dei_cmds_;
  std::pair<std::vector<DrawCmdUniformModelOnly>, std::vector<uint32_t>>
      chunk_frame_uniforms_mesh_ids_;
  // std::vector<uint32_t> chunk_frame_draw_cmd_mesh_ids_;
  // std::vector<ChunkDrawCmdUniform> chunk_frame_draw_cmd_uniforms_;
  std::vector<DrawElementsIndirectCommand> chunk_frame_dei_cmds_;
  bool static_chunk_buffer_dirty_{true};
  bool lod_static_chunk_buffer_dirty_{true};

  VertexArray reg_mesh_vao_;
  DynamicBuffer<> reg_mesh_vbo_;
  DynamicBuffer<> reg_mesh_ebo_;
  Buffer reg_mesh_uniform_ssbo_;
  Buffer reg_mesh_draw_indirect_buffer_;
  std::unordered_map<uint32_t, MeshAlloc> reg_mesh_allocs_;
  std::unordered_map<uint32_t, DrawElementsIndirectCommand> reg_mesh_dei_cmds_;
  std::pair<std::vector<UniformsModelMaterial>, std::vector<uint32_t>>
      reg_mesh_frame_uniforms_mesh_ids_;
  std::vector<DrawElementsIndirectCommand> frame_dei_cmd_vec_;

  DynamicBuffer<> tex_materials_buffer_;
  std::unordered_map<uint32_t, uint32_t> material_allocs_;

  // std::vector<DrawCmdUniform> quad_frame_draw_cmd_uniforms_;
  VertexArray quad_vao_;
  Buffer quad_vbo_;
  Buffer quad_ebo_;
  VertexArray full_screen_quad_vao_;
  Buffer full_screen_quad_vbo_;
  Buffer full_screen_quad_ebo_;

  VertexArray skybox_vao_;
  Buffer skybox_vbo_;

  Buffer textured_quad_uniform_ssbo_;
  std::vector<UniformsModelMaterial> quad_textured_uniforms_;
  std::vector<UniformsModelMaterial> static_textured_quad_uniforms_;

  Buffer color_uniform_ssbo_;
  std::vector<UniformsModelColor> quad_color_uniforms_;
  std::pair<std::vector<UniformsModelColor>, std::vector<uint32_t>> lines_frame_uniforms_mesh_ids_;
  std::pair<std::vector<UniformsModelColor>, std::vector<uint32_t>>
      lines_no_depth_frame_uniforms_mesh_ids_;

  VertexArray cube_vao_;
  Buffer cube_vbo_;
  Buffer cube_ebo_;

  std::function<void()> draw_skyox_func_;

  void LoadShaders();

  void SetDrawIndirectBufferData(
      Buffer& draw_indirect_buffer, std::vector<uint32_t>& frame_draw_cmd_mesh_ids,
      std::vector<DrawElementsIndirectCommand>& frame_dei_cmds,
      std::unordered_map<uint32_t, DrawElementsIndirectCommand>& dei_cmds);
  void ShutdownInternal();
  void HandleResize(int new_width, int new_height);
  void InitFrameBuffers();

  struct Stats {
    uint32_t textured_quad_draw_calls{0};
    uint32_t color_quad_draw_calls{0};
    uint32_t total_chunk_vertices{0};
    uint32_t total_chunk_indices{0};
    uint32_t lod_chunk_vertices{0};
    uint32_t lod_chunk_indices{0};
    uint32_t total_reg_mesh_vertices{0};
    uint32_t total_reg_mesh_indices{0};
  };
  Stats stats_;
};
