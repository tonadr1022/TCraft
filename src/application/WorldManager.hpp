#pragma once

#include <unordered_set>
struct WorldCreateParams {
  std::string name;
  std::string seed;
};

class WorldManager {
 public:
  WorldManager();
  void CreateWorldStructure(const WorldCreateParams& params);
  void DeleteWorld(std::string_view name);
  bool WorldExists(const std::string& name);
  ~WorldManager() = default;

 private:
  std::unordered_set<std::string> world_names_;
};
