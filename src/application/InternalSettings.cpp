#include "InternalSettings.hpp"

#include <imgui.h>

#include <fstream>
#include <glm/trigonometric.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

InternalSettings* InternalSettings::instance_ = nullptr;
InternalSettings& InternalSettings::Get() { return *instance_; }
InternalSettings::InternalSettings() {
  EASSERT_MSG(instance_ == nullptr, "Cannot create two instances.");
  instance_ = this;
}

void InternalSettings::Load(const std::string& path) {
  std::ifstream f(path);
  json s = json::parse(f);
  default_load_distance = s["default_load_distance"];
}

void InternalSettings::Save(const std::string& path) {
  std::ofstream f(path);
  json s = {
      {"default_load_distance", default_load_distance},
  };
  std::ofstream o(path);
  o << s << std::endl;
}

void InternalSettings::OnImGui() {
  if (ImGui::CollapsingHeader("InternalSettings", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::SliderInt("Default Load Distance", &default_load_distance, 1, 32.0);
  }
}
