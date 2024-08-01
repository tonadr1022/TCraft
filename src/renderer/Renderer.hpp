#pragma once

#include <SDL_events.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include "renderer/ChunkRenderParams.hpp"
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

class WorldScene;
class BlockEditorScene;
class Window;
class BlockDB;
struct Vertex;
struct ChunkVertex;
struct TextureMaterial;

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
  void SubmitRegMeshDrawCommand(const glm::mat4& model, uint32_t mesh_handle,
                                uint32_t material_handle);
  void SubmitQuadDrawCommand(const glm::mat4& model, uint32_t material_handle);
  [[nodiscard]] uint32_t AllocateMesh(std::vector<ChunkVertex>& vertices,
                                      std::vector<uint32_t>& indices);

  [[nodiscard]] uint32_t AllocateMesh(std::vector<Vertex>& vertices,
                                      std::vector<uint32_t>& indices);

  void FreeChunkMesh(uint32_t handle);
  void FreeRegMesh(uint32_t handle);
  [[nodiscard]] uint32_t AllocateMaterial(TextureMaterial& material);
  void FreeMaterial(uint32_t material_handle);

  void Init();
  void StartFrame(const Window& window);
  [[nodiscard]] bool OnEvent(const SDL_Event& event);
  void Shutdown();

 private:
  friend class Application;
  static Renderer* instance_;
  Renderer();

  constexpr const static uint32_t MaxDrawCmds{1'000'000};
  ShaderManager shader_manager_;

  Mesh quad_mesh_;

  // TODO: try without alignas
  struct alignas(16) DrawCmdUniform {
    glm::mat4 model;
    uint64_t material_index;
  };

  struct MeshAlloc {
    uint32_t vbo_handle;
    uint32_t ebo_handle;
  };

  VertexArray chunk_vao_;
  DynamicBuffer chunk_vbo_;
  DynamicBuffer chunk_ebo_;
  Buffer chunk_uniform_ssbo_;
  Buffer chunk_draw_indirect_buffer_;

  VertexArray reg_mesh_vao_;
  DynamicBuffer reg_mesh_vbo_;
  DynamicBuffer reg_mesh_ebo_;
  Buffer reg_mesh_uniform_ssbo_;
  Buffer reg_mesh_draw_indirect_buffer_;

  DynamicBuffer tex_materials_buffer_;

  std::unordered_map<uint32_t, MeshAlloc> chunk_allocs_;
  std::unordered_map<uint32_t, DrawElementsIndirectCommand> chunk_dei_cmds_;

  std::unordered_map<uint32_t, MeshAlloc> reg_mesh_allocs_;
  std::unordered_map<uint32_t, DrawElementsIndirectCommand> reg_mesh_dei_cmds_;

  std::vector<uint32_t> chunk_frame_draw_cmd_mesh_ids_;
  std::vector<DrawCmdUniform> chunk_frame_draw_cmd_uniforms_;
  std::vector<DrawElementsIndirectCommand> chunk_frame_dei_cmds_;

  std::vector<uint32_t> reg_mesh_frame_draw_cmd_mesh_ids_;
  std::vector<DrawCmdUniform> reg_mesh_frame_draw_cmd_uniforms_;
  std::vector<DrawElementsIndirectCommand> reg_mesh_frame_dei_cmds_;

  std::unordered_map<uint32_t, uint32_t> material_allocs_;

  void LoadShaders();
  void SetChunkFrameDrawCommands();
  void SetRegMeshFrameDrawCommands();
};
