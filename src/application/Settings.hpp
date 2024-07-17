#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Settings {
 public:
  static Settings& Get();
  void OnImGui();
  void Load(const std::string& path);
  void Save(const std::string& path);
  // Must be called before main Save(path)
  void SaveSetting(json& j, std::string_view name);
  // Must be called after main Load(path)
  [[nodiscard]] json LoadSetting(std::string_view name) const;

  // main settings in the settings object
  float mouse_sensitivity;
  float fps_cam_fov_deg;
  bool imgui_enabled;
  bool wireframe_enabled;
  int load_distance;
  ~Settings() = default;

 private:
  json settings_json_;
  friend class Application;
  Settings();
  static Settings* instance_;
};
