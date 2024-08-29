#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class SettingsManager {
 public:
  static SettingsManager& Get();
  static void Init();
  static void Shutdown();
  [[nodiscard]] uint32_t CoreCount() const;
  void OnImGui();
  void Load(const std::string& path);
  void Shutdown(const std::string& path);
  // Must be called before main Save(path)
  void SaveSetting(json& j, std::string_view name);
  // Must be called after main Load(path)
  [[nodiscard]] json LoadSetting(std::string_view name) const;

  // main settings in the settings object
  float mouse_sensitivity{1};
  float fov_degrees{75};
  float orbit_mouse_sensitivity{1};
  const uint32_t core_count_;
  ~SettingsManager() = default;

 private:
  json settings_json_;
  SettingsManager();
  static SettingsManager* instance_;
};
