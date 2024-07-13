#pragma once

class Settings {
 public:
  static Settings& Get();
  float mouse_sensitivity{1.f};
  float fps_cam_fov_deg{90};

 private:
  friend class Application;
  Settings();
  static Settings* instance_;
};
