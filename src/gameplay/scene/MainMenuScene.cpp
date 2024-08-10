#include "MainMenuScene.hpp"

#include <imgui.h>

#include "application/SceneManager.hpp"
#include "application/WorldManager.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/Terrain.hpp"

namespace {
void ClearField(char* data, size_t size) { memset(data, '\0', size); }
}  // namespace

MainMenuScene::MainMenuScene(SceneManager& scene_manager)
    : Scene(scene_manager), world_manager_(std::make_unique<WorldManager>()) {
  BlockDB block_db;
  block_db.Init();
  Terrain ter;
  ter.Load(block_db);
}

MainMenuScene::~MainMenuScene() = default;

void MainMenuScene::OnImGui() {
  ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
  {
    static bool create_open = false;
    if (create_open) {
      static char world_name[128] = "";
      static char seed[128] = "";
      ImGui::InputText("World Name", world_name, IM_ARRAYSIZE(world_name));
      ImGui::InputText("Seed", seed, IM_ARRAYSIZE(seed));

      bool world_with_name_exists = world_manager_->WorldExists(world_name);
      if (world_with_name_exists) {
        ImGui::Text("World with name already exists");
      }

      if (ImGui::Button("Cancel")) {
        create_open = false;
        ClearField(world_name, sizeof(world_name));
        ClearField(seed, sizeof(seed));
      }
      ImGui::SameLine();
      ImGui::BeginDisabled(world_with_name_exists || !strcmp(world_name, ""));
      if (ImGui::Button("Create")) {
        if (world_manager_->WorldExists(world_name)) {
        } else {
          world_manager_->CreateWorld({.name = world_name, .seed = seed});
        }
      }
      ImGui::EndDisabled();

    } else {
      if (ImGui::Button("Create World")) create_open = true;
    }
  }
  {
    if (ImGui::Button("Block Editor")) {
      scene_manager_.LoadScene("block_editor");
    }
  }
  ImGui::End();
}

bool MainMenuScene::OnEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_p && event.key.keysym.mod & KMOD_ALT) {
        scene_manager_.LoadWorld("default");
        return true;
      }
  }
  return false;
}
