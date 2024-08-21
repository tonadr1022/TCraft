

#include <glm/vec2.hpp>
struct UIBlockData {
  uint32_t id;
  glm::vec2 pos;
  uint32_t mesh_handle{0};
};

// void InventoryUI::RenderAndWriteIcons() {
//   const auto& data = block_db_.GetBlockData();
//   const auto& mesh_data = block_db_.GetMeshData();
//   Buffer vbo;
//   Buffer ebo;
//   vbo.Init(sizeof(ChunkVertex) * 1000, GL_DYNAMIC_STORAGE_BIT);
//   ebo.Init(sizeof(uint32_t) * 1000, GL_DYNAMIC_STORAGE_BIT);
//   VertexArray vao;
//   vao.Init();
//   vao.EnableAttribute<uint32_t>(0, 2, 0);
//   vao.AttachVertexBuffer(vbo.Id(), 0, 0, sizeof(ChunkVertex));
//   vao.AttachElementBuffer(ebo.Id());
//   glm::ivec2 dims = {512, 512};
//   uint32_t tex, rbo, fbo;
//
//   // make rbo
//   glCreateRenderbuffers(1, &rbo);
//   glNamedRenderbufferStorage(rbo, GL_DEPTH24_STENCIL8, dims.x, dims.y);
//
//   // make fbo
//   glCreateFramebuffers(1, &fbo);
//   glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
//
//   if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
//     spdlog::error("Frambuffer incomplete");
//   }
//
//   vao.Bind();
//   const auto& tex_arr = TextureManager::Get().GetTexture2dArray(tex_array_);
//   auto shader = ShaderManager::Get().GetShader("block");
//   shader->Bind();
//   std::vector<uint8_t> pixels(dims.x * dims.y * 4);
//   glm::mat4 perspective_proj = glm::perspective(glm::radians(75.f), 1.f, 0.01f, 1000.f);
//   // glm::mat4 ortho_proj = glm::ortho(0.0f, static_cast<float>(dims.x), 0.0f,
//   //                                   static_cast<float>(dims.y), 0.001f, 100.0f);
//   glm::mat4 ortho_proj = glm::ortho(-1.f, 1.f, -1.f, 1.f, -100.f, 100.0f);
//   glm::mat4 view_matrix =
//       glm::lookAt(glm::vec3(1),                 // Camera position, slightly above and to the
//       side
//                   glm::vec3(0.0f, 0.0f, 0.0f),  // Look at the origin
//                   glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
//       );
//   glm::mat4 vp_matrix = ortho_proj * view_matrix;
//   shader->SetMat4("vp_matrix", vp_matrix);
//   std::vector<ChunkVertex> vertices;
//   std::vector<uint32_t> indices;
//   tex_arr.Bind(0);
//   for (size_t j = 1; j < block_data_.size(); j++) {
//     const auto& d = block_data_[j];
//     // make tex
//     glCreateTextures(GL_TEXTURE_2D, 1, &tex);
//     glTextureStorage2D(tex, 1, GL_RGBA8, dims.x, dims.y);
//     glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     // attach tex
//     glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, tex, 0);
//     glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//     glViewport(0, 0, dims.x, dims.y);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     ChunkMesher mesher{data, mesh_data};
//     ChunkMesher::GenerateBlock(vertices, indices, mesh_data[d.id].texture_indices);
//     vbo.SubDataStart(sizeof(ChunkVertex) * vertices.size(), vertices.data());
//     ebo.SubDataStart(sizeof(uint32_t) * indices.size(), indices.data());
//     glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
//     std::string out_path =
//         GET_PATH("resources/icons/") + block_db_.GetBlockData()[d.id].name + ".png";
//     util::WriteImage(tex, 4, out_path);
//
//     glDeleteTextures(1, &tex);
//     vertices.clear();
//     indices.clear();
//   }
//
//   glDeleteFramebuffers(1, &fbo);
//   glDeleteRenderbuffers(1, &rbo);
//   glBindFramebuffer(GL_FRAMEBUFFER, 0);
//   auto win_dims = Window::Get().GetWindowSize();
//   glViewport(0, 0, win_dims.x, win_dims.y);
// }
