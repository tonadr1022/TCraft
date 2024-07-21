#include "Renderer.hpp"

#include "ShaderManager.hpp"
#include "application/Settings.hpp"
#include "application/Window.hpp"
#include "gameplay/scene/WorldScene.hpp"
#include "pch.hpp"
#include "renderer/opengl/Buffer.hpp"
#include "renderer/opengl/Debug.hpp"
#include "resource/TextureManager.hpp"
#include "util/Paths.hpp"

void Renderer::Init() {
  auto settings = Settings::Get().LoadSetting("renderer");
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, nullptr);
  LoadShaders();
  wireframe_enabled_ = settings.value("wireframe_enabled", false);
}
void Renderer::Shutdown() {
  nlohmann::json settings;
  settings["wireframe_enabled"] = wireframe_enabled_;
  Settings::Get().SaveSetting(settings, "renderer");
}

void Renderer::RenderWorld(const WorldScene& world, const RenderInfo& render_info,
                           const BlockDB& db) {
  // TODO: cleanup rendering
  glViewport(0, 0, render_info.window_dims.x, render_info.window_dims.y);
  glClearColor(1, 0.0, 0.6, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // TODO: separate wireframe into renderer
  glPolygonMode(GL_FRONT_AND_BACK, wireframe_enabled_ ? GL_LINE : GL_FILL);

  auto chunk_shader = shader_manager_.GetShader("chunk");
  chunk_shader->Bind();
  chunk_shader->SetMat4("vp_matrix", render_info.vp_matrix);
  chunk_shader->SetInt("u_Texture", 0);
  {
    ZoneScopedN("Chunk render");
    auto& chunk_tex_arr =
        TextureManager::Get().GetTexture2dArray(world.world_render_params_.chunk_tex_array_handle);
    glBindTextureUnit(0, chunk_tex_arr.Id());
    int bound_id;
    glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &bound_id);
    // spdlog::info("{} {}", chunk_tex_arr.Id(), bound_id);
    // prints "1 0"

    // TODO: only send to renderer the chunks ready to be rendered instead of the whole map
    for (const auto& it : world.chunk_map_) {
      auto& mesh = it.second->mesh;
      glm::vec3 pos = it.first * ChunkLength;
      chunk_shader->SetMat4("model_matrix", glm::translate(glm::mat4{1}, pos));
      mesh.vao.Bind();
      glDrawElements(GL_TRIANGLES, mesh.num_indices_, GL_UNSIGNED_INT, nullptr);
    }
  }
}
void Renderer::Render(const Window& window) {
  auto dims = window.GetWindowSize();
  glViewport(0, 0, dims.x, dims.y);
  glClearColor(1, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPolygonMode(GL_FRONT_AND_BACK, wireframe_enabled_ ? GL_LINE : GL_FILL);
}

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
  shader_manager_.AddShader("chunk", {{GET_SHADER_PATH("chunk.vs.glsl"), ShaderType::Vertex},
                                      {GET_SHADER_PATH("chunk.fs.glsl"), ShaderType::Fragment}});
}
