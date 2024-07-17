#include "WorldScene.hpp"

#include <memory>

#include "gameplay/world/BlockDB.hpp"

void WorldScene::Init() { block_db_ = std::make_unique<BlockDB>(); }

void WorldScene::Update(double dt) {}

bool WorldScene::OnEvent(const SDL_Event& event) { return true; }

void WorldScene::OnImGui() const {}
