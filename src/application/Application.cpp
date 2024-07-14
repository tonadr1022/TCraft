#include "Application.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_timer.h>
#include <imgui.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <string>

#include "Settings.hpp"
#include "Window.hpp"
#include "application/Input.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/World.hpp"
#include "renderer/Cube.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/ShaderManager.hpp"
#include "renderer/opengl/Buffer.hpp"
#include "renderer/opengl/VertexArray.hpp"
#include "util/Paths.hpp"

namespace {

constexpr const auto SettingsPath = GET_PATH("resources/settings.json");
struct Vertex {
  glm::vec3 position;
  glm::vec2 tex_coords;
};

}  // namespace

Application::Application(int width, int height, const char* title) {
  // load settings first
  settings_ = new Settings;

  window_.Init(width, height, title, [this](SDL_Event& event) { OnEvent(event); });

  // Create singletons
  // TODO(tony): implement config file
  Settings::Get().LoadSettings(SettingsPath);
  shader_manager_ = new ShaderManager;
  renderer_ = new Renderer;

  // Add event listeners
  event_dispatcher_.AddListener(
      [this](const SDL_Event& event) { return Renderer::Get().OnEvent(event); });
  event_dispatcher_.AddListener([this](const SDL_Event& event) { return player_.OnEvent(event); });

  Renderer::Get().Init();
  player_.Init();
}

void Application::Run() {
  World world;
  world.Load(GET_PATH("resources/worlds/world_default"));

  VertexArray vao;
  vao.EnableAttribute<float>(0, 3, offsetof(Vertex, position));
  vao.EnableAttribute<float>(1, 2, offsetof(Vertex, tex_coords));
  auto cube_vertex_buffer =
      std::make_unique<Buffer>(sizeof(CubeVertices), 0, (void*)CubeVertices.data());
  auto cube_index_buffer =
      std::make_unique<Buffer>(sizeof(CubeIndices), 0, (void*)CubeIndices.data());
  cube_vertex_buffer->Bind(GL_ARRAY_BUFFER);
  cube_index_buffer->Bind(GL_ELEMENT_ARRAY_BUFFER);
  vao.AttachVertexBuffer(cube_vertex_buffer->Id(), 0, 0, sizeof(Vertex));
  vao.AttachElementBuffer(cube_index_buffer->Id());

  Uint64 curr_time = SDL_GetPerformanceCounter();
  Uint64 prev_time = 0;
  double dt = 0;
  // create vao
  while (!window_.ShouldClose()) {
    prev_time = curr_time;
    curr_time = SDL_GetPerformanceCounter();
    dt = ((curr_time - prev_time) * 1000 / static_cast<double>(SDL_GetPerformanceFrequency()));
    window_.StartFrame();
    player_.Update(dt);
    auto dims = window_.GetWindowSize();
    glm::mat4 vp_matrix =
        player_.GetCamera().GetProjection(static_cast<float>(dims.y) / static_cast<float>(dims.x)) *
        player_.GetCamera().GetView();

    glViewport(0, 0, dims.x, dims.y);
    glClearColor(1, 0.6, 0.6, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto color = ShaderManager::Get().GetShader("color");
    color->Bind();
    color->SetVec3("color", {0, 1, 0});
    color->SetMat4("model_matrix", glm::mat4{1});
    color->SetMat4("vp_matrix", vp_matrix);
    vao.Bind();
    glDrawElements(GL_TRIANGLES, CubeIndices.size(), GL_UNSIGNED_INT, nullptr);

    if (Settings::Get().imgui_enabled) OnImGui();
    window_.EndFrame();
  }

  window_.Shutdown();
}

Application::~Application() {
  delete renderer_;
  delete shader_manager_;
  Settings::Get().SaveSettings(SettingsPath);
  delete settings_;
}

void Application::OnEvent(const SDL_Event& event) {
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

void Application::OnImGui() {
  player_.OnImGui();
  ImGuiIO& io = ImGui::GetIO();
  static int counter = 0;
  ImGui::Begin("App", nullptr, ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoFocusOnAppearing);
  ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
  Settings::Get().OnImGui();
  ImGui::End();
}
