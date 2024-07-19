#include "MainMenuScene.hpp"

#include <imgui.h>

#include "application/SceneManager.hpp"
#include "renderer/Renderer.hpp"

MainMenuScene::MainMenuScene(SceneManager& scene_manager) : Scene(scene_manager) {}

void MainMenuScene::Update(double dt) {}

void MainMenuScene::OnImGui() {
  ImGui::Begin("Main Menu Scene");
  ImGui::Text("test");
  ImGui::End();
}

bool MainMenuScene::OnEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_p && event.key.keysym.mod & KMOD_ALT) {
        scene_manager_.LoadScene("world");
        return true;
      }
  }
  return false;
}

void MainMenuScene::Render(Renderer& renderer, const Window& window) { renderer.Render(window); }
