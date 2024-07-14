#include "Settings.hpp"

#include <imgui.h>

#include <glm/trigonometric.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Settings* Settings::instance_ = nullptr;
Settings& Settings::Get() { return *instance_; }
Settings::Settings() {
  EASSERT_MSG(instance_ == nullptr, "Cannot create two settings instances.");
  instance_ = this;
}

void Settings::LoadSettings(const std::string& path) {
  std::ifstream f(path);
  json s = json::parse(f);
  mouse_sensitivity = s["mouse_sensitivity"].get<float>();
  fps_cam_fov_deg = s["fps_cam_fov_deg"].get<float>();
  imgui_enabled = s["imgui_enabled"].get<bool>();
}

void Settings::SaveSettings(const std::string& path) {
  std::ofstream f(path);
  json s = {
      {"mouse_sensitivity", mouse_sensitivity},
      {"fps_cam_fov_deg", fps_cam_fov_deg},
      {"imgui_enabled", imgui_enabled},
  };
  std::ofstream o(path);
  o << s << std::endl;
}

void Settings::OnImGui() {
  if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::SliderFloat("Mouse Sensitivity", &mouse_sensitivity, 0.2, 1.0);
    float fps_cam_fov_rad = glm::radians(fps_cam_fov_deg);
    if (ImGui::SliderAngle("FPS Camera FOV", &fps_cam_fov_rad, 45, 120)) {
      fps_cam_fov_deg = glm::degrees(fps_cam_fov_rad);
    }
  }
}
