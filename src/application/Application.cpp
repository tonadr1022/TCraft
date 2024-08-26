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
#include "renderer/ShaderManager.hpp"
#include "resource/MaterialManager.hpp"
#include "resource/TextureManager.hpp"
#include "util/Paths.hpp"

namespace {

constexpr const auto kSettingsPath = GET_PATH("resources/settings.json");

}  // namespace

Application::Application(int width, int height, const char* title) : scene_manager_(window_) {
  SettingsManager::Init();
  TextureManager::Init();
  MaterialManager::Init();
  ShaderManager::Init();
  SettingsManager::Get().Load(kSettingsPath);

  auto app_settings_json = SettingsManager::Get().LoadSetting("application");
  imgui_enabled_ = app_settings_json.value("imgui_enabled", true);
  window_.Init(width, height, title, [this](SDL_Event& event) { OnEvent(event); });
  Renderer::Init(window_);

  // Add event listeners
  event_dispatcher_.AddListener(
      [](const SDL_Event& event) { return Renderer::Get().OnEvent(event); });
  event_dispatcher_.AddListener(
      [this](const SDL_Event& event) { return scene_manager_.GetActiveScene().OnEvent(event); });
}

void Application::Run() {
  ZoneScoped;
  // scene_manager_.LoadScene("main_menu");
  // scene_manager_.LoadScene("block_editor");
  scene_manager_.LoadWorld(GET_PATH("resources/worlds/default_world"));

  Uint64 curr_time = SDL_GetPerformanceCounter();
  Uint64 prev_time = 0;
  double dt = 0;
  // create vao
  while (!window_.ShouldClose()) {
    ZoneScopedN("main loop");
    prev_time = curr_time;
    curr_time = SDL_GetPerformanceCounter();
    dt = ((curr_time - prev_time) / static_cast<double>(SDL_GetPerformanceFrequency()));

    window_.PollEvents();
    scene_manager_.GetActiveScene().Update(dt);

    {
      ZoneScopedN("Render");
      window_.StartRenderFrame(imgui_enabled_);
      Renderer::Get().StartFrame(window_);
      scene_manager_.GetActiveScene().Render();

      if (imgui_enabled_) OnImGui();
      window_.EndRenderFrame(imgui_enabled_);
    }
  }

  nlohmann::json app_settings_json = {{"imgui_enabled", imgui_enabled_}};
  SettingsManager::Get().SaveSetting(app_settings_json, "application");

  scene_manager_.Shutdown();
  ShaderManager::Shutdown();
  MaterialManager::Shutdown();
  TextureManager::Shutdown();
  Renderer::Shutdown();
  window_.Shutdown();
  // TODO: cleanup
  SettingsManager::Get().Shutdown(kSettingsPath);
  SettingsManager::Shutdown();
}

Application::~Application() { ZoneScoped; };

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
    case SDL_MOUSEBUTTONDOWN:
      Input::SetMouseButtonPressed(event.button.button, true);
      break;
    case SDL_MOUSEBUTTONUP:
      Input::SetMouseButtonPressed(event.button.button, false);
      break;
  }
}

void Application::OnImGui() {
  ZoneScoped;
  ImGuiIO& io = ImGui::GetIO();
  ImGui::Begin("App", nullptr, ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoFocusOnAppearing);
  ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
  bool vsync = window_.GetVsync();
  if (ImGui::Checkbox("Vsync", &vsync)) {
    window_.SetVsync(vsync);
  }
  static bool fullscreen = false;
  if (ImGui::Checkbox("Fullscreen", &fullscreen)) {
    window_.SetFullScreen(fullscreen);
  }

  if (ImGui::Button("Load world")) {
    scene_manager_.LoadWorld(GET_PATH("resources/worlds/default_world"));
  }
  if (ImGui::Button("Block Editor")) {
    scene_manager_.LoadScene("block_editor");
  }
  if (ImGui::Button("Main Menu")) {
    scene_manager_.LoadScene("main_menu");
  }

  SettingsManager::Get().OnImGui();
  Renderer::Get().OnImGui();
  scene_manager_.GetActiveScene().OnImGui();
  ImGui::End();
}
