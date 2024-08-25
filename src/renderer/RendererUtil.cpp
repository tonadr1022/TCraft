
#include "RendererUtil.hpp"

#include <cstddef>
#include <filesystem>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/vec2.hpp>

#include "application/Window.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkDef.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/ShaderManager.hpp"
#include "renderer/TextureAtlas.hpp"
#include "renderer/opengl/Buffer.hpp"
#include "renderer/opengl/Texture2d.hpp"
#include "renderer/opengl/VertexArray.hpp"
#include "resource/MaterialManager.hpp"
#include "util/LoadFile.hpp"
#include "util/Paths.hpp"

namespace util::renderer {

// TODO: separate block data struct and block mesh data struct to own file
void RenderAndWriteIcons(const std::vector<BlockData>& data,
                         const std::vector<BlockMeshData>& mesh_data, const Texture& tex_arr) {
  Buffer vbo;
  Buffer ebo;
  vbo.Init(sizeof(ChunkVertex) * 24, GL_DYNAMIC_STORAGE_BIT);
  ebo.Init(sizeof(uint32_t) * 36, GL_DYNAMIC_STORAGE_BIT);
  VertexArray vao;
  vao.Init();
  vao.EnableAttribute<uint32_t>(0, 2, 0);
  vao.AttachVertexBuffer(vbo.Id(), 0, 0, sizeof(ChunkVertex));
  vao.AttachElementBuffer(ebo.Id());
  glm::ivec2 dims = {512, 512};
  // TODO: make raii fbo wrapper
  uint32_t tex, rbo, fbo;

  // make rbo
  glCreateRenderbuffers(1, &rbo);
  glNamedRenderbufferStorage(rbo, GL_DEPTH24_STENCIL8, dims.x, dims.y);

  // make fbo
  glCreateFramebuffers(1, &fbo);
  glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

  if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    spdlog::error("Frambuffer incomplete");
  }

  vao.Bind();
  auto shader = ShaderManager::Get().GetShader("block");
  shader->Bind();
  std::vector<uint8_t> pixels(static_cast<size_t>(dims.x * dims.y * 4));
  glm::mat4 proj = glm::ortho(-1.f, 1.f, -1.f, 1.f, -100.f, 100.0f);
  glm::mat4 view =
      glm::lookAt(glm::vec3(1), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  shader->SetMat4("vp_matrix", proj * view);
  std::vector<ChunkVertex> vertices;
  std::vector<uint32_t> indices;
  tex_arr.Bind(0);
  if (!std::filesystem::exists(GET_PATH("resources/icons"))) {
    std::filesystem::create_directory(GET_PATH("resources/icons"));
  }
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glViewport(0, 0, dims.x, dims.y);
  for (size_t j = 1; j < data.size(); j++) {
    const auto& d = data[j];
    // make tex
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, dims.x, dims.y);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // attach tex
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ChunkMesher mesher{data, mesh_data};
    ChunkMesher::GenerateBlock(vertices, indices, mesh_data[d.id].texture_indices);
    vbo.SubDataStart(sizeof(ChunkVertex) * vertices.size(), vertices.data());
    ebo.SubDataStart(sizeof(uint32_t) * indices.size(), indices.data());
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
    std::filesystem::path out_path =
        std::filesystem::path(GET_PATH("resources/icons")) / (d.name + ".png");
    // std::string out_path = GET_PATH("resources/icons/") + d.name + ".png";
    util::WriteImage(tex, 4, out_path);

    glDeleteTextures(1, &tex);
    vertices.clear();
    indices.clear();
  }

  glDeleteFramebuffers(1, &fbo);
  glDeleteRenderbuffers(1, &rbo);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  auto win_dims = Window::Get().GetWindowSize();
  glViewport(0, 0, win_dims.x, win_dims.y);
}

extern void RenderAndWriteIcon(const std::string& path, const BlockMeshData& mesh_data,
                               const Texture& tex_arr) {
  Buffer vbo;
  Buffer ebo;
  vbo.Init(sizeof(ChunkVertex) * 24, GL_DYNAMIC_STORAGE_BIT);
  ebo.Init(sizeof(uint32_t) * 36, GL_DYNAMIC_STORAGE_BIT);
  VertexArray vao;
  vao.Init();
  vao.EnableAttribute<uint32_t>(0, 2, 0);
  vao.AttachVertexBuffer(vbo.Id(), 0, 0, sizeof(ChunkVertex));
  vao.AttachElementBuffer(ebo.Id());
  glm::ivec2 dims = {128, 128};
  // TODO: make raii fbo wrapper
  uint32_t tex, rbo, fbo;

  // make rbo
  glCreateRenderbuffers(1, &rbo);
  glNamedRenderbufferStorage(rbo, GL_DEPTH24_STENCIL8, dims.x, dims.y);

  // make fbo
  glCreateFramebuffers(1, &fbo);
  glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

  if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    spdlog::error("Frambuffer incomplete");
  }

  vao.Bind();
  auto shader = ShaderManager::Get().GetShader("block");
  shader->Bind();
  std::vector<uint8_t> pixels(static_cast<size_t>(dims.x * dims.y * 4));
  float rad = 0.85f;
  glm::mat4 proj = glm::ortho(-rad, rad, -rad, rad, -100.f, 100.0f);
  glm::mat4 view =
      glm::lookAt(glm::vec3(1), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  shader->SetMat4("vp_matrix", proj * view);
  std::vector<ChunkVertex> vertices;
  std::vector<uint32_t> indices;
  tex_arr.Bind(0);
  if (!std::filesystem::exists(GET_PATH("resources/icons"))) {
    std::filesystem::create_directory(GET_PATH("resources/icons"));
  }
  // make tex
  glCreateTextures(GL_TEXTURE_2D, 1, &tex);
  glTextureStorage2D(tex, 1, GL_RGBA8, dims.x, dims.y);
  glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // attach tex
  glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, tex, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glViewport(0, 0, dims.x, dims.y);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ChunkMesher::GenerateBlock(vertices, indices, mesh_data.texture_indices);
  vbo.SubDataStart(sizeof(ChunkVertex) * vertices.size(), vertices.data());
  ebo.SubDataStart(sizeof(uint32_t) * indices.size(), indices.data());
  glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
  // std::string out_path = GET_PATH("resources/icons/") + d.name + ".png";
  util::WriteImage(tex, 4, path);

  glDeleteTextures(1, &tex);
  vertices.clear();
  indices.clear();

  glDeleteFramebuffers(1, &fbo);
  glDeleteRenderbuffers(1, &rbo);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  auto win_dims = Window::Get().GetWindowSize();
  glViewport(0, 0, win_dims.x, win_dims.y);
}

void LoadIcons(std::vector<Image>& images) {
  images.clear();
  for (const auto& file : std::filesystem::directory_iterator(GET_PATH("resources/icons"))) {
    Image img;
    util::LoadImage(img, file.path().string(), 0);
    images.emplace_back(img);
  }
}

SquareTextureAtlas LoadIconTextureAtlas(const BlockDB& block_db, const Texture& tex_arr) {
  SquareTextureAtlas res;

  // Load icons into atlas for ImGui
  std::vector<std::pair<uint32_t, Image>> images;
  for (const auto& data : block_db.GetBlockData()) {
    if (data.id == 0) continue;
    Image img;
    std::filesystem::path path =
        GET_PATH("resources/icons") / std::filesystem::path(data.name + ".png");
    if (!std::filesystem::exists(path)) {
      util::renderer::RenderAndWriteIcon(path, block_db.GetMeshData()[data.id], tex_arr);
    }
    util::LoadImage(img, path.string(), 0);
    images.emplace_back(data.id, img);
  }

  uint32_t num_textures_wide = glm::ceil(glm::sqrt(images.size()));
  res.dims.x = num_textures_wide * images.front().second.width;
  res.dims.y = num_textures_wide * images.front().second.height;
  res.image_dims.x = images.front().second.width;
  res.image_dims.y = images.front().second.height;

  res.material = MaterialManager::Get().LoadTextureMaterial(
      "icons",
      Texture2DCreateParamsEmpty{static_cast<uint32_t>(res.dims.x),
                                 static_cast<uint32_t>(res.dims.y), GL_LINEAR, GL_LINEAR, true});
  size_t i = 0;
  for (uint32_t row = 0; row < num_textures_wide; row++) {
    for (uint32_t col = 0; col < num_textures_wide; col++, i++) {
      if (i >= images.size()) break;
      uint32_t x_offset = row * res.image_dims.x;
      uint32_t y_offset = col * res.image_dims.y;
      // spdlog::info("{}", block_db_.GetBlockData()[images[i].first].name);
      glTextureSubImage2D(res.material->GetTexture().Id(), 0, x_offset, y_offset, res.image_dims.x,
                          res.image_dims.y, GL_RGBA, GL_UNSIGNED_BYTE, images[i].second.pixels);
      res.id_to_offset_map.emplace(images[i].first, glm::vec2{x_offset, y_offset});
    }
  }

  for (const auto& image : images) {
    if (image.second.pixels) util::FreeImage(image.second.pixels);
  }
  return res;
}

}  // namespace util::renderer
