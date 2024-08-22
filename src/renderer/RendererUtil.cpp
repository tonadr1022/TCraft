
#include "RendererUtil.hpp"

#include <filesystem>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/vec2.hpp>

#include "application/Window.hpp"
#include "gameplay/world/BlockDB.hpp"
#include "gameplay/world/ChunkDef.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/ShaderManager.hpp"
#include "renderer/opengl/Buffer.hpp"
#include "renderer/opengl/Texture2d.hpp"
#include "renderer/opengl/VertexArray.hpp"
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
  std::vector<uint8_t> pixels(dims.x * dims.y * 4);
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
    glViewport(0, 0, dims.x, dims.y);
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

}  // namespace util::renderer
