#pragma once

class Texture2D {
 public:
  explicit Texture2D(const std::string& path);

  void Bind() const;

 private:
  int id_{0};
};
