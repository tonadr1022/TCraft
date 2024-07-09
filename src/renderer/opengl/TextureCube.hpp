#pragma once

class TextureCube {
 public:
  TextureCube(std::vector<std::string>& paths);
  void Bind() const;

 private:
  int id_{0};
};
