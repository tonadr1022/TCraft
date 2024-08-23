#pragma once

#include "renderer/opengl/Shader.hpp"

enum class ShaderType { kVertex, kFragment, kGeometry, kCompute };

struct ShaderCreateInfo {
  std::string shaderPath;
  ShaderType shaderType;
};

class ShaderManager {
 public:
  static void Init();
  static void Shutdown();
  static ShaderManager& Get();
  std::optional<Shader> GetShader(const std::string& name);
  std::optional<Shader> AddShader(const std::string& name,
                                  const std::vector<ShaderCreateInfo>& create_info_vec);
  std::optional<Shader> RecompileShader(const std::string& name);
  void RecompileShaders();

 private:
  static ShaderManager* instance_;
  ShaderManager();

  struct ShaderProgramData {
    std::string name;
    uint32_t program_id;
    std::unordered_map<std::string, uint32_t> uniform_locations;
    std::vector<ShaderCreateInfo> create_info_vec;

    void InitializeUniforms();
  };

  std::optional<ShaderProgramData> CompileProgram(
      const std::string& name, const std::vector<ShaderCreateInfo>& create_info_vec);
  std::unordered_map<std::string, ShaderProgramData> shader_data_;
};
