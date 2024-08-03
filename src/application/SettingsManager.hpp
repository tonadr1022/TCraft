#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class SettingsManager {
 public:
  static SettingsManager& Get();
  void OnImGui();
  void Load(const std::string& path);
  void Shutdown(const std::string& path);
  // Must be called before main Save(path)
  void SaveSetting(json& j, std::string_view name);
  // Must be called after main Load(path)
  [[nodiscard]] json LoadSetting(std::string_view name) const;

  // main settings in the settings object
  float mouse_sensitivity;
  float fps_cam_fov_deg;
  float orbit_mouse_sensitivity;
  int load_distance;
  ~SettingsManager() = default;

 private:
  json settings_json_;
  friend class Application;
  SettingsManager();
  static SettingsManager* instance_;
};
