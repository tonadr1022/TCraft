#pragma once

#include <SDL_events.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include "renderer/ShaderManager.hpp"
#include "renderer/opengl/Buffer.hpp"

struct DrawElementsIndirectCommand {
  uint32_t count;
  uint32_t instance_count;
  uint32_t first_index;
  int base_vertex;
  uint32_t base_instance;
};

class WorldScene;
class Window;
class BlockDB;

struct RenderInfo {
  glm::ivec2 window_dims;
  glm::mat4 vp_matrix;
};

class Renderer {
 public:
  bool render_gui_{true};
  bool render_chunks_{true};
  bool wireframe_enabled_{false};

  void RenderWorld(const WorldScene& world, const RenderInfo& render_info);
  void Render(const Window& window) const;
  void Init();
  bool OnEvent(const SDL_Event& event);
  void Shutdown();
  ~Renderer();

 private:
  ShaderManager shader_manager_;

  std::unique_ptr<Buffer> vertex_buffer_{nullptr};
  std::unique_ptr<Buffer> element_buffer_{nullptr};
  void LoadShaders();
};
