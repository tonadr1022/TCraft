#include "BlockEditorScene.hpp"

BlockEditorScene::BlockEditorScene(SceneManager& scene_manager) : Scene(scene_manager) {}

void BlockEditorScene::OnImGui() {}

bool BlockEditorScene::OnEvent(const SDL_Event& /*event*/) { return false; }

BlockEditorScene::~BlockEditorScene() = default;
