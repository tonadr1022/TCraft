#pragma once

// Lightweight object containing id and reference to uniform locations stored in the manager.
// This is a wrapper to access the shader and set uniforms
using Float4Arr = float[4];
class Shader {
 public:
  void Bind() const;
  static void Unbind();

  void SetInt(const std::string& name, int value);
  void SetFloat(const std::string& name, float value);
  void SetMat4(const std::string& name, const glm::mat4& mat);
  void SetIVec2(const std::string& name, const glm::ivec2& vec);
  void SetIVec3(const std::string& name, const glm::ivec3& vec);
  void SetVec3(const std::string& name, const glm::vec3& vec);
  void SetVec2(const std::string& name, const glm::vec2& vec);
  void SetVec4(const std::string& name, const glm::vec4& vec);
  void SetVec4(const std::string& name, const Float4Arr& vec);
  void SetMat3(const std::string& name, const glm::mat3& mat, bool transpose = false);
  void SetBool(const std::string& name, bool value);
  void SetFloatArr(const std::string& name, GLuint count, const GLfloat* value);

  Shader(uint32_t id, std::unordered_map<std::string, uint32_t>& uniform_locations);
  ~Shader() = default;
  [[nodiscard]] inline uint32_t Id() const { return id_; }

 private:
  uint32_t id_{0};
  std::unordered_map<std::string, uint32_t>& uniform_locations_;
};
