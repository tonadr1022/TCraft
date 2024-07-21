#include "Application.hpp"

#include <SDL2/SDL_timer.h>
#include <imgui.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>

#include "Settings.hpp"
#include "Window.hpp"
#include "application/Input.hpp"
#include "renderer/Renderer.hpp"
#include "util/Paths.hpp"

namespace {

constexpr const auto SettingsPath = GET_PATH("resources/settings.json");

}  // namespace

Application::Application(int width, int height, const char* title) {
  // Settings is a singleton since only one instance should exist
  settings_ = new Settings;
  Settings::Get().Load(SettingsPath);
  auto app_settings_json = Settings::Get().LoadSetting("application");
  imgui_enabled_ = app_settings_json.value("imgui_enabled", true);

  window_.Init(width, height, title, [this](SDL_Event& event) { OnEvent(event); });
  renderer_.Init();

  // Add event listeners
  event_dispatcher_.AddListener(
      [this](const SDL_Event& event) { return renderer_.OnEvent(event); });
  event_dispatcher_.AddListener(
      [this](const SDL_Event& event) { return scene_manager_.GetActiveScene().OnEvent(event); });
}

void Application::Run() {
  ZoneScoped;
  scene_manager_.LoadScene("main_menu");
  // scene_manager_.LoadWorld("default");

  RenderInfo render_info;
  Uint64 curr_time = SDL_GetPerformanceCounter();
  Uint64 prev_time = 0;
  double dt = 0;
  // create vao
  while (!window_.ShouldClose()) {
    ZoneScopedN("main loop");
    prev_time = curr_time;
    curr_time = SDL_GetPerformanceCounter();
    dt = ((curr_time - prev_time) * 1000 / static_cast<double>(SDL_GetPerformanceFrequency()));

    window_.PollEvents();
    scene_manager_.GetActiveScene().Update(dt);

    {
      ZoneScopedN("Render");
      window_.StartRenderFrame(imgui_enabled_);
      scene_manager_.GetActiveScene().Render(renderer_, window_);

      if (imgui_enabled_) OnImGui();
      window_.EndRenderFrame(imgui_enabled_);
    }
  }

  nlohmann::json app_settings_json = {{"imgui_enabled", imgui_enabled_}};
  Settings::Get().SaveSetting(app_settings_json, "application");

  renderer_.Shutdown();
  scene_manager_.Shutdown();
  window_.Shutdown();
}

Application::~Application() { Settings::Get().Shutdown(SettingsPath); }

void Application::OnEvent(const SDL_Event& event) {
  ZoneScoped;

  if (event.key.keysym.sym == SDLK_g && event.key.keysym.mod & KMOD_ALT) {
    imgui_enabled_ = !imgui_enabled_;
    return;
  }
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
  ZoneScoped;
  ImGuiIO& io = ImGui::GetIO();
  static int counter = 0;
  ImGui::Begin("App", nullptr, ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoFocusOnAppearing);
  ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
  bool vsync = window_.GetVsync();
  if (ImGui::Checkbox("Vsync", &vsync)) {
    window_.SetVsync(vsync);
  }

  Settings::Get().OnImGui();
  scene_manager_.GetActiveScene().OnImGui();
  ImGui::End();
}
