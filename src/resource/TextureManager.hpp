#pragma once

using TextureHandle = uint32_t;
class Texture;
struct Texture2DArrayCreateParams;
struct TextureCubeCreateParamsPaths;
struct Texture2DCreateParams;
struct Texture2DCreateParamsEmpty;

class TextureManager {
 public:
  static void Init();
  static void Shutdown();
  static TextureManager& Get();
  void Erase(const std::string& name);

  [[nodiscard]] std::shared_ptr<Texture> Load(const Texture2DArrayCreateParams& params);
  [[nodiscard]] std::shared_ptr<Texture> Load(const std::string& name,
                                              const TextureCubeCreateParamsPaths& params);
  [[nodiscard]] std::shared_ptr<Texture> Load(const Texture2DCreateParams& params);
  [[nodiscard]] std::shared_ptr<Texture> Load(const std::string& name,
                                              const Texture2DCreateParamsEmpty& params);
  void RemoveUnusedTextures();

 private:
  static TextureManager* instance_;
  TextureManager();
  uint32_t next_tex_array_handle_{1};

  std::unordered_map<std::string, std::shared_ptr<Texture>> texture_map_;
};
