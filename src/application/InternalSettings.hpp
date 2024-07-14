#pragma once

class InternalSettings {
 public:
  static InternalSettings& Get();
  void OnImGui();
  void Load(const std::string& path);
  void Save(const std::string& path);
  int default_load_distance;

 private:
  friend class Application;
  InternalSettings();
  static InternalSettings* instance_;
};
