#include "ShaderManager.hpp"

#include <fstream>

#include "util/Paths.hpp"

ShaderManager *ShaderManager::instance_ = nullptr;

ShaderManager &ShaderManager::Get() { return *instance_; }

void ShaderManager::Init() {
  EASSERT_MSG(instance_ == nullptr, "Can't make two instances");
  instance_ = new ShaderManager;
}

void ShaderManager::Shutdown() {
  ZoneScoped;
  EASSERT_MSG(instance_ != nullptr, "Can't shutdown before initializing");
  delete instance_;
}

ShaderManager::ShaderManager() {
  EASSERT_MSG(instance_ == nullptr, "Cannot create two instances.");
  instance_ = this;
}

std::optional<Shader> ShaderManager::GetShader(const std::string &name) {
  auto it = shader_data_.find(name);
  if (it == shader_data_.end()) {
    spdlog::error("Shader not found {}", name.data());
    return std::nullopt;
  }
  return Shader{it->second.program_id, it->second.uniform_locations};
}

std::optional<Shader> ShaderManager::AddShader(
    const std::string &name, const std::vector<ShaderCreateInfo> &create_info_vec) {
  auto result = ShaderManager::CompileProgram(name, create_info_vec);
  if (!result.has_value()) {
    return std::nullopt;
  }
  // spdlog::info("Compiled shader: {}, Id: {}", name, result->program_id);
  shader_data_.emplace(name.data(), result.value());
  return Shader{result->program_id, result->uniform_locations};
}

bool CheckShaderModuleCompilationSuccess(uint32_t shader_id, const char *shaderPath) {
  int success;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
  if (!success) {
    char buffer[512];
    glGetShaderInfoLog(shader_id, 512, nullptr, buffer);
    spdlog::error("Error compiling shader file: {}\n{}", shaderPath, buffer);
    return false;
  }
  return true;
}

static constexpr GLenum kShaderTypeToGl[] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER,
                                             GL_GEOMETRY_SHADER, GL_COMPUTE_SHADER};

bool CheckProgramLinkSuccess(GLuint id) {
  glLinkProgram(id);
  int success;
  glGetProgramiv(id, GL_LINK_STATUS, &success);
  if (!success) {
    char buffer[512];
    glGetProgramInfoLog(id, 512, nullptr, buffer);
    spdlog::error("Shader Link error {}", buffer);
    return false;
  }
  return true;
}

uint32_t CompileShader(ShaderType type, const char *src) {
  uint32_t id = glCreateShader(kShaderTypeToGl[static_cast<int>(type)]);
  glShaderSource(id, 1, &src, nullptr);
  glCompileShader(id);
  return id;
}

namespace {

std::optional<std::string> PreprocessShader(const std::string &shader_file) {
  std::ifstream file(shader_file);
  if (!file.is_open()) {
    return {};
  }

  std::ostringstream content;
  std::string line;

  while (std::getline(file, line)) {
    if (line.find("#include") == 0) {
      size_t start = line.find('\"') + 1;
      size_t end = line.find('\"', start);
      std::string include_file = line.substr(start, end - start);
      std::string full_path = GET_SHADER_PATH("") + include_file;

      // TODO: fix macro or make another way to get true path since this is hacky way to get
      // macro to work
      auto include_content = PreprocessShader(full_path);
      if (!include_content.has_value()) {
        return {};
      }
      content << include_content.value();
    } else {
      content << line << '\n';
    }
  }

  return content.str();
}
}  // namespace

std::optional<ShaderManager::ShaderProgramData> ShaderManager::CompileProgram(
    const std::string &name, const std::vector<ShaderCreateInfo> &create_info_vec) {
  std::vector<uint32_t> shader_ids;
  for (const auto &create_info : create_info_vec) {
    auto src = PreprocessShader(create_info.shaderPath);
    if (!src.has_value()) {
      spdlog::error("Failed to load from file {}", create_info.shaderPath);
      return std::nullopt;
    }
    uint32_t shader_id = CompileShader(create_info.shaderType, src.value().c_str());
    if (!CheckShaderModuleCompilationSuccess(shader_id, create_info.shaderPath.c_str())) {
      spdlog::error("error: {}", create_info.shaderPath.c_str());
      return std::nullopt;
    }
    shader_ids.push_back(shader_id);
  }

  uint32_t program_id = glCreateProgram();
  for (auto &shader_id : shader_ids) {
    glAttachShader(program_id, shader_id);
  }
  glLinkProgram(program_id);
  if (!CheckProgramLinkSuccess(program_id)) {
    return std::nullopt;
  }

  ShaderProgramData data;
  data.program_id = program_id;
  data.name = name;
  data.create_info_vec = create_info_vec;
  data.InitializeUniforms();
  return data;
}

std::optional<Shader> ShaderManager::RecompileShader(const std::string &name) {
  auto it = shader_data_.find(name);
  if (it == shader_data_.end()) {
    spdlog::warn("Shader not found, cannot recompile: {}", name.data());
    return std::nullopt;
  }
  auto recompile_result = CompileProgram(name, it->second.create_info_vec);
  if (!recompile_result.has_value()) {
    return std::nullopt;
  }
  spdlog::info("Shader recompiled: {}", name.data());
  shader_data_.erase(it);
  auto new_it = shader_data_.emplace(name, recompile_result.value());
  return Shader{new_it.first->second.program_id, new_it.first->second.uniform_locations};
}

void ShaderManager::ShaderProgramData::InitializeUniforms() {
  EASSERT_MSG(program_id != 0, "Can't initialize uniforms on invalid shader");
  GLint active_uniform_max_length;
  glGetProgramiv(program_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &active_uniform_max_length);
  GLint num_uniforms;
  glGetProgramiv(program_id, GL_ACTIVE_UNIFORMS, &num_uniforms);

  GLenum uniform_type;
  GLint uniform_size;
  GLint uniform_name_length;
  char uniform_name[100];
  for (int i = 0; i < num_uniforms; i++) {
    glGetActiveUniform(program_id, i, active_uniform_max_length, &uniform_name_length,
                       &uniform_size, &uniform_type, uniform_name);
    uint32_t location = glGetUniformLocation(program_id, uniform_name);
    uniform_locations.emplace(uniform_name, location);
  }
}

void ShaderManager::RecompileShaders() {
  // have to avoid iterator invalidation
  std::vector<std::string> shader_names;
  shader_names.reserve(shader_data_.size());
  for (auto &shader_data : shader_data_) {
    shader_names.emplace_back(shader_data.second.name.data());
  }
  for (const auto &shader_name : shader_names) {
    RecompileShader(shader_name);
  }
}
