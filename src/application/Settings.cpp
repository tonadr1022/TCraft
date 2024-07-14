#include "Settings.hpp"

#include <imgui.h>

#include <fstream>
#include <glm/trigonometric.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
Settings* Settings::instance_ = nullptr;
Settings& Settings::Get() { return *instance_; }
Settings::Settings() {
  EASSERT_MSG(instance_ == nullptr, "Cannot create two settings instances.");
  instance_ = this;
}

void Settings::Load(const std::string& path) {
  std::ifstream f(path);
  settings_json_ = json::parse(f);

  // Main settings
  auto& main = settings_json_["main"];
  mouse_sensitivity = main["mouse_sensitivity"].get<float>();
  fps_cam_fov_deg = main["fps_cam_fov_deg"].get<float>();
  imgui_enabled = main["imgui_enabled"].get<bool>();
  wireframe_enabled = main["wireframe_enabled"].get<bool>();
}

void Settings::Save(const std::string& path) {
  std::ofstream f(path);

  // save main settings
  settings_json_["main"] = {
      {"mouse_sensitivity", mouse_sensitivity}, {"fps_cam_fov_deg", fps_cam_fov_deg},
      {"imgui_enabled", imgui_enabled},         {"wireframe_enabled", wireframe_enabled},
      {"load_distance", load_distance},
  };
  std::ofstream o(path);
  o << std::setw(2) << settings_json_ << std::endl;
}

json Settings::LoadSetting(std::string_view name) const { return settings_json_[name]; }

void Settings::SaveSetting(json& j, std::string_view name) { settings_json_[name] = j; }

void Settings::OnImGui() {
  if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::SliderFloat("Mouse Sensitivity", &mouse_sensitivity, 0.2, 1.0);
    float fps_cam_fov_rad = glm::radians(fps_cam_fov_deg);
    if (ImGui::SliderAngle("FPS Camera FOV", &fps_cam_fov_rad, 45, 120)) {
      fps_cam_fov_deg = glm::degrees(fps_cam_fov_rad);
    }
  }
}
