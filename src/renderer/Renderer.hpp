#pragma once

#include <SDL_events.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include "renderer/ShaderManager.hpp"

struct DrawElementsIndirectCommand {
  uint32_t count;
  uint32_t instance_count;
  uint32_t first_index;
  int base_vertex;
  uint32_t base_instance;
};

class Buffer;
class World;

struct RenderInfo {
  glm::ivec2 window_dims;
  glm::mat4 vp_matrix;
};

class Renderer {
 public:
  bool render_gui_{true};
  bool render_chunks_{true};

  void Render(World& world, const RenderInfo& render_info);
  void Init();
  bool OnEvent(const SDL_Event& event);

 private:
  ShaderManager shader_manager_;

  std::unique_ptr<Buffer> vertex_buffer_{nullptr};
  std::unique_ptr<Buffer> element_buffer_{nullptr};
  void LoadShaders();
};
