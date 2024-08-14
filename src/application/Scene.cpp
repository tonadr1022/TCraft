#include "Scene.hpp"

#include "application/SceneManager.hpp"
#include "renderer/Renderer.hpp"

void Scene::Render() {}

void Scene::Update(double) {}

void Scene::OnImGui() {}

bool Scene::OnEvent(const SDL_Event&) { return false; }

Scene::~Scene() { Renderer::Get().RemoveStaticMeshes(); }

Scene::Scene(SceneManager& scene_manager)
    : scene_manager_(scene_manager), window_(scene_manager_.window_) {}
