#include "SettingsManager.hpp"

#include <imgui.h>

#include <glm/trigonometric.hpp>
#include <nlohmann/json.hpp>

#include "util/JsonUtil.hpp"
#include "util/LoadFile.hpp"

using json = nlohmann::json;
SettingsManager* SettingsManager::instance_ = nullptr;
SettingsManager& SettingsManager::Get() { return *instance_; }

SettingsManager::SettingsManager() : core_count_(std::thread::hardware_concurrency()) {
  EASSERT_MSG(instance_ == nullptr, "Cannot create two instances.");
  instance_ = this;
}
void SettingsManager::Init() {
  EASSERT_MSG(instance_ == nullptr, "Can't make two instances");
  instance_ = new SettingsManager;
}

void SettingsManager::Shutdown() {
  ZoneScoped;
  EASSERT_MSG(instance_ != nullptr, "Can't shutdown before initializing");
  delete instance_;
}

void SettingsManager::Load(const std::string& path) {
  settings_json_ = util::LoadJsonFile(path);
  if (!settings_json_.is_object()) return;

  // Main settings
  auto& main = settings_json_["main"];
  mouse_sensitivity = main.value("mouse_sensitivity", 1.0f);
  fov_degrees = main.value("fps_cam_fov_deg", 75.f);
  orbit_mouse_sensitivity = main.value("orbit_mouse_sensitivity", 1.f);
}

void SettingsManager::Shutdown(const std::string& path) {
  ZoneScoped;
  // save main settings
  settings_json_["main"] = {{"mouse_sensitivity", mouse_sensitivity},
                            {"fps_cam_fov_deg", fov_degrees},
                            {"orbit_mouse_sensitivity", orbit_mouse_sensitivity}};
  util::json::WriteJson(settings_json_, path);
}

json SettingsManager::LoadSetting(std::string_view name) const {
  if (settings_json_.contains(name)) {
    return settings_json_[name];
  }
  return json::object();
}

void SettingsManager::SaveSetting(json& j, std::string_view name) { settings_json_[name] = j; }

void SettingsManager::OnImGui() {
  if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::SliderFloat("Mouse Sensitivity", &mouse_sensitivity, 0.2, 3.0);
    ImGui::SliderFloat("Orbit Mouse Sensitivity", &orbit_mouse_sensitivity, 0.2, 1.0);
    float fps_cam_fov_rad = glm::radians(fov_degrees);
    if (ImGui::SliderAngle("FPS Camera FOV", &fps_cam_fov_rad, 45, 120)) {
      fov_degrees = glm::degrees(fps_cam_fov_rad);
    }
  }
}

uint32_t SettingsManager::CoreCount() const { return core_count_; }
