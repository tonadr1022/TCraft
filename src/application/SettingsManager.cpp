#include "SettingsManager.hpp"

#include <imgui.h>

#include <glm/trigonometric.hpp>
#include <nlohmann/json.hpp>

#include "util/JsonUtil.hpp"
#include "util/LoadFile.hpp"

using json = nlohmann::json;
SettingsManager* SettingsManager::instance_ = nullptr;
SettingsManager& SettingsManager::Get() { return *instance_; }
SettingsManager::SettingsManager() {
  EASSERT_MSG(instance_ == nullptr, "Cannot create two instances.");
  instance_ = this;
}

void SettingsManager::Load(const std::string& path) {
  settings_json_ = util::LoadJsonFile(path);

  // Main settings
  auto& main = settings_json_["main"];
  mouse_sensitivity = main["mouse_sensitivity"].get<float>();
  fps_cam_fov_deg = main["fps_cam_fov_deg"].get<float>();
}

void SettingsManager::Shutdown(const std::string& path) {
  // save main settings
  settings_json_["main"] = {
      {"mouse_sensitivity", mouse_sensitivity},
      {"fps_cam_fov_deg", fps_cam_fov_deg},
      {"load_distance", load_distance},
  };
  json_util::WriteJson(settings_json_, path);
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
    ImGui::SliderFloat("Mouse Sensitivity", &mouse_sensitivity, 0.2, 1.0);
    float fps_cam_fov_rad = glm::radians(fps_cam_fov_deg);
    if (ImGui::SliderAngle("FPS Camera FOV", &fps_cam_fov_rad, 45, 120)) {
      fps_cam_fov_deg = glm::degrees(fps_cam_fov_rad);
    }
  }
}
