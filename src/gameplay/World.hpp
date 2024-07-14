#pragma once

#include <string_view>

class World {
 public:
  void Update(double dt);
  void Load(std::string_view path);

 private:
};
