#include "Application.hpp"

#include <SDL2/SDL_timer.h>
#include <SDL_events.h>
#include <imgui.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>

#include "SettingsManager.hpp"
#include "Window.hpp"
#include "application/Input.hpp"
#include "application/SceneManager.hpp"
#include "renderer/Renderer.hpp"
#include "resource/TextureManager.hpp"
#include "util/Paths.hpp"

namespace {

constexpr const auto SettingsPath = GET_PATH("resources/settings.json");

}  // namespace

Application::Application(int width, int height, const char* title) {
  // initialize singletons
  settings_ = new SettingsManager;
  texture_manager_ = new TextureManager;
  renderer_ = new Renderer;

  SettingsManager::Get().Load(SettingsPath);
  auto app_settings_json = SettingsManager::Get().LoadSetting("application");
  imgui_enabled_ = app_settings_json.value("imgui_enabled", true);

  window_.Init(width, height, title, [this](SDL_Event& event) { OnEvent(event); });
  Renderer::Get().Init();

  // Add event listeners
  event_dispatcher_.AddListener(
      [this](const SDL_Event& event) { return Renderer::Get().OnEvent(event); });
  event_dispatcher_.AddListener(
      [this](const SDL_Event& event) { return scene_manager_.GetActiveScene().OnEvent(event); });
}

void Application::Run() {
  ZoneScoped;
  // scene_manager_.LoadScene("main_menu");
  scene_manager_.LoadScene("block_editor");
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
      Renderer::Get().StartFrame(window_);
      scene_manager_.GetActiveScene().Render(window_);

      if (imgui_enabled_) OnImGui();
      window_.EndRenderFrame(imgui_enabled_);
    }
  }

  nlohmann::json app_settings_json = {{"imgui_enabled", imgui_enabled_}};
  SettingsManager::Get().SaveSetting(app_settings_json, "application");

  scene_manager_.Shutdown();
  Renderer::Get().Shutdown();
  window_.Shutdown();
}

Application::~Application() {
  SettingsManager::Get().Shutdown(SettingsPath);
  delete renderer_;
  delete texture_manager_;
  delete settings_;
}

void Application::OnEvent(const SDL_Event& event) {
  ZoneScoped;

  if (event.type == SDL_KEYDOWN) {
    if (event.key.keysym.sym == SDLK_g && event.key.keysym.mod & KMOD_ALT) {
      imgui_enabled_ = !imgui_enabled_;
      return;
    }
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
  if (ImGui::Button("Load world")) {
    scene_manager_.LoadWorld("default_world");
  }
  if (ImGui::Button("Block Editor")) {
    scene_manager_.LoadScene("block_editor");
  }
  if (ImGui::Button("Main Menu")) {
    scene_manager_.LoadScene("main_menu");
  }

  SettingsManager::Get().OnImGui();
  scene_manager_.GetActiveScene().OnImGui();
  ImGui::End();
}
