#include "Application.hpp"

#include <imgui.h>

#include "application/Window.hpp"

Application::Application(int width, int height, const char* title) {
  window_.Init(width, height, title, [this](SDL_Event& event) { OnEvent(event); });
}

void Application::Run() {
  while (!window_.ShouldClose()) {
    window_.StartFrame();

    ImGuiIO& io = ImGui::GetIO();

    glViewport(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    glClearColor(0.6, 0.6, 0.6, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    static int counter = 0;

    ImGui::Begin("Hello, world!");
    ImGui::Text("This is some useful text.");
    if (ImGui::Button("Button")) counter++;

    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate,
                io.Framerate);
    ImGui::End();

    window_.EndFrame();
  }

  window_.Shutdown();
}

void Application::OnEvent(SDL_Event& event) { spdlog::info(event.type); }
