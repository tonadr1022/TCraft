#include "Renderer.hpp"

#include "ShaderManager.hpp"
#include "application/Settings.hpp"
#include "gameplay/world/World.hpp"
#include "pch.hpp"
#include "renderer/opengl/Buffer.hpp"
#include "renderer/opengl/Debug.hpp"
#include "util/Paths.hpp"

void Renderer::Init() {
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, nullptr);
  LoadShaders();
}

void Renderer::Render(World& world, const RenderInfo& render_info) {
  // TODO: cleanup rendering
  glViewport(0, 0, render_info.window_dims.x, render_info.window_dims.y);
  glClearColor(1, 0.0, 0.6, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPolygonMode(GL_FRONT_AND_BACK, Settings::Get().wireframe_enabled ? GL_LINE : GL_FILL);

  auto color = shader_manager_.GetShader("color");
  color->Bind();
  color->SetMat4("vp_matrix", render_info.vp_matrix);

  {
    ZoneScopedN("Chunk render");

    // TODO: only send to renderer the chunks ready to be rendered instead of the whole map
    for (const auto& it : world.chunk_map_) {
      auto& mesh = it.second->mesh;
      glm::vec3 pos = it.first * ChunkLength;

      color->SetVec3("color", {rand() % 1, rand() % 1, rand() % 1});
      color->SetMat4("model_matrix", glm::translate(glm::mat4{1}, pos));
      mesh.vao.Bind();
      glDrawElements(GL_TRIANGLES, mesh.num_indices_, GL_UNSIGNED_INT, nullptr);
    }
  }
}

bool Renderer::OnEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_r && event.key.keysym.mod & KMOD_ALT) {
        shader_manager_.RecompileShaders();
        return true;
      }
  }
  return false;
}

void Renderer::LoadShaders() {
  shader_manager_.AddShader("color", {{GET_SHADER_PATH("color.vs.glsl"), ShaderType::Vertex},
                                      {GET_SHADER_PATH("color.fs.glsl"), ShaderType::Fragment}});
}
