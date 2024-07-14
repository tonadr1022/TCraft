#pragma once

class Settings {
 public:
  static Settings& Get();
  void OnImGui();
  void LoadSettings(const std::string& path);
  void SaveSettings(const std::string& path);
  float mouse_sensitivity;
  float fps_cam_fov_deg;
  bool imgui_enabled;

 private:
  friend class Application;
  Settings();
  static Settings* instance_;
};
