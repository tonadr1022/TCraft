#include "Renderer.hpp"

#include "ShaderManager.hpp"
#include "pch.hpp"
#include "renderer/opengl/Buffer.hpp"
#include "renderer/opengl/Debug.hpp"
#include "util/Paths.hpp"

Renderer* Renderer::instance_{nullptr};
Renderer& Renderer::Get() { return *instance_; }

void Renderer::Init() {
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, nullptr);
  LoadShaders();
}

Renderer::Renderer() {
  EASSERT_MSG(instance_ == nullptr, "Cannot create two renderers .");
  instance_ = this;
}

void Renderer::Render() {}

Renderer::~Renderer() = default;

void Renderer::LoadShaders() {
  ShaderManager::Get().AddShader("color",
                                 {{GET_SHADER_PATH("color.vs.glsl"), ShaderType::Vertex},
                                  {GET_SHADER_PATH("color.fs.glsl"), ShaderType::Fragment}});
}

bool Renderer::OnEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_r && event.key.keysym.mod & KMOD_ALT) {
        ShaderManager::Get().RecompileShaders();
        return true;
      }
  }
  return false;
}
