#pragma once

#include <SDL_events.h>

struct DrawElementsIndirectCommand {
  uint32_t count;
  uint32_t instance_count;
  uint32_t first_index;
  int base_vertex;
  uint32_t base_instance;
};

class Buffer;

class Renderer {
 public:
  bool render_gui_{true};
  bool render_chunks_{true};

  void Render();
  void Init();
  bool OnEvent(const SDL_Event& event);

  static Renderer& Get();
  ~Renderer();

 private:
  friend class Application;
  Renderer();
  static Renderer* instance_;

  std::unique_ptr<Buffer> vertex_buffer_{nullptr};
  std::unique_ptr<Buffer> element_buffer_{nullptr};
  void LoadShaders();
};
