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
#include "application/InternalSettings.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "renderer/Cube.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/ShaderManager.hpp"
#include "renderer/opengl/Buffer.hpp"
#include "renderer/opengl/VertexArray.hpp"
#include "resource/ResourceLoader.hpp"
#include "util/Paths.hpp"

namespace {

constexpr const auto SettingsPath = GET_PATH("resources/settings.json");
constexpr const auto InternalSettingsPath = GET_PATH("resources/internal_settings.json");
struct Vertex {
  glm::vec3 position;
  glm::vec2 tex_coords;
};

}  // namespace

Application::Application(int width, int height, const char* title) {
  // load settings first
  settings_ = new Settings;
  internal_settings_ = new InternalSettings;
  block_db_ = new BlockDB;

  window_.Init(width, height, title, [this](SDL_Event& event) { OnEvent(event); });
  ResourceLoader::LoadResources();
  BlockDB::Get().LoadData();

  // Create singletons
  // TODO(tony): implement config file
  Settings::Get().Load(SettingsPath);
  InternalSettings::Get().Load(InternalSettingsPath);
  shader_manager_ = new ShaderManager;
  renderer_ = new Renderer;

  // Add event listeners
  event_dispatcher_.AddListener(
      [this](const SDL_Event& event) { return Renderer::Get().OnEvent(event); });
  event_dispatcher_.AddListener([this](const SDL_Event& event) { return world_.OnEvent(event); });

  Renderer::Get().Init();
}

RenderInfo render_info;
void Application::Run() {
  ZoneScoped;
  world_.Load(GET_PATH("resources/worlds/world_default"));

  Uint64 curr_time = SDL_GetPerformanceCounter();
  Uint64 prev_time = 0;
  double dt = 0;
  // create vao
  while (!window_.ShouldClose()) {
    ZoneScopedN("main loop");
    //////////////////////////// DELTA TIME ///////////////////////////////
    prev_time = curr_time;
    curr_time = SDL_GetPerformanceCounter();
    dt = ((curr_time - prev_time) * 1000 / static_cast<double>(SDL_GetPerformanceFrequency()));

    /////////////////////////////// UPDATE ////////////////////////////////////////
    window_.PollEvents();
    world_.Update(dt);

    ////////////////////////// RENDERING ///////////////////////////////////
    {
      ZoneScopedN("Render");
      window_.StartRenderFrame();
      auto& player = world_.GetPlayer();
      render_info.window_dims = window_.GetWindowSize();
      float aspect_ratio = static_cast<float>(render_info.window_dims.x) /
                           static_cast<float>(render_info.window_dims.y);
      render_info.vp_matrix =
          player.GetCamera().GetProjection(aspect_ratio) * player.GetCamera().GetView();
      Renderer::Get().Render(world_, render_info);

      {
        ZoneScopedN("ImGui calls");
        if (Settings::Get().imgui_enabled) OnImGui();
      }
      window_.EndRenderFrame();
    }
  }

  /////////////////////////// SHUTDOWN ////////////////////////////////////////
  world_.Save();
  window_.Shutdown();
}

Application::~Application() {
  delete renderer_;
  delete shader_manager_;
  Settings::Get().Save(SettingsPath);
  InternalSettings::Get().Save(InternalSettingsPath);
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
  ImGuiIO& io = ImGui::GetIO();
  static int counter = 0;
  ImGui::Begin("App", nullptr, ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoFocusOnAppearing);
  ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
  bool vsync = window_.GetVsync();
  if (ImGui::Checkbox("Vsync", &vsync)) {
    window_.SetVsync(vsync);
  }

  Settings::Get().OnImGui();
  world_.OnImGui();
  ImGui::End();
}
