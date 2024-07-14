#pragma once

/* clang-format off */
const std::array<float, 120> CubeVertices= {
      // front face
      1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
      1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f,

      // back face
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
      0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 1.0f, 1.0f, 1.0f,

      // left face
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
      1.0f, 0.0f, 1.0f, 1.0f, 1.0f,

      // right face
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
      1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f,

      // top face
      0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f,

      // bottom face
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
      1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
      1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
  };

  const std::array<unsigned int, 36> CubeIndices = {
      0, 1, 2, 2, 1, 3, // front face
      4, 5, 6, 6, 5, 7, // back face
      8, 9, 10, 10, 9, 11, // left face
      12, 13, 14, 14, 13, 15, // right face
      16, 17, 18, 18, 17, 19, // top face
      20, 21, 22, 22, 21, 23, // bottom face
  };
/* clang-format on */
