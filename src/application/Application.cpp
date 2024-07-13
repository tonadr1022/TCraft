#include "Application.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <imgui.h>

#include <glm/vec3.hpp>
#include <memory>
#include <string>

#include "Settings.hpp"
#include "Window.hpp"
#include "application/Input.hpp"
#include "renderer/Cube.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/ShaderManager.hpp"
#include "renderer/opengl/Buffer.hpp"
#include "renderer/opengl/VertexArray.hpp"
#include "util/Paths.hpp"

Application::Application(int width, int height, const char* title) {
  window_.Init(width, height, title, [this](SDL_Event& event) { OnEvent(event); });

  // Create singletons
  // TODO(tony): implement config file
  settings_ = new Settings;
  shader_manager_ = new ShaderManager;
  renderer_ = new Renderer;

  // Add event listeners
  event_dispatcher_.AddListener(
      [this](const SDL_Event& event) { return Renderer::Get().OnEvent(event); });

  Renderer::Get().Init();
}

struct Vertex {
  glm::vec3 position;
};

void Application::Run() {
  VertexArray vao;
  vao.EnableAttribute<float>(0, 3, offsetof(Vertex, position));
  auto cube_vertex_buffer =
      std::make_unique<Buffer>(sizeof(CubeVertices), 0, (void*)CubeVertices.data());
  auto cube_index_buffer =
      std::make_unique<Buffer>(sizeof(CubeIndices), 0, (void*)CubeIndices.data());
  vao.AttachVertexBuffer(cube_vertex_buffer->Id(), 0, 0, sizeof(glm::vec3));
  vao.AttachElementBuffer(cube_index_buffer->Id());

  // create vao
  while (!window_.ShouldClose()) {
    window_.StartFrame();

    ImGuiIO& io = ImGui::GetIO();
    glViewport(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    glClearColor(0.6, 0.6, 0.6, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    auto color = ShaderManager::Get().GetShader("color");
    color->Bind();

    ImGuiTest();

    window_.EndFrame();
  }

  window_.Shutdown();
}

Application::~Application() {
  delete renderer_;
  delete shader_manager_;
  delete settings_;
}

void Application::OnEvent(SDL_Event& event) {
  if (event_dispatcher_.Dispatch(event)) return;
  switch (event.type) {
    case SDL_KEYDOWN:
      Input::SetKeyPressed(event.key.keysym.sym, true);
      break;
    case SDL_KEYUP:
      Input::SetKeyPressed(event.key.keysym.sym, false);
      break;
  }
}

void Application::ImGuiTest() {
  ImGuiIO& io = ImGui::GetIO();
  static int counter = 0;
  ImGui::Begin("Hello, world!");
  ImGui::Text("This is some useful text.");
  if (ImGui::Button("Button")) counter++;
  ImGui::SameLine();
  ImGui::Text("counter = %d", counter);
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
  ImGui::End();
}

void Application::LoadShaders() {
  ShaderManager::Get().AddShader("color",
                                 {{GET_SHADER_PATH("color.vs.glsl"), ShaderType::Vertex},
                                  {GET_SHADER_PATH("color.fs.glsl"), ShaderType::Fragment}});
}
