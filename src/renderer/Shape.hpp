#pragma once

/* clang-format off */
#include "renderer/Vertex.hpp"
const std::array<float, 120> CubeVertices= {
      // right face
      1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
      1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f,

      // left face
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
      0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 1.0f, 1.0f, 1.0f,


      // top face
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
      1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f,

      // bottom face
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
      1.0f, 0.0f, 1.0f, 1.0f, 1.0f,

      // front face
      0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f,

      // back face
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
      1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
      1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
  };

  const std::array<uint32_t, 36> CubeIndices = {
      0, 1, 2, 2, 1, 3, // front face
      4, 5, 6, 6, 5, 7, // back face
      8, 9, 10, 10, 9, 11, // left face
      12, 13, 14, 14, 13, 15, // right face
      16, 17, 18, 18, 17, 19, // top face
      20, 21, 22, 22, 21, 23, // bottom face
  };
/* clang-format on */

constexpr const float CubePositionVertices[] = {
    // positions
    -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
    -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

    1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

const std::array<Vertex, 4> QuadVertices = {{// positions        // texture Coords
                                             {glm::vec3{0.0f, 1.0f, 0.0f}, glm::vec2{0.0f, 1.0f}},
                                             {glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec2{0.0f, 0.0f}},
                                             {glm::vec3{1.0f, 1.0f, 0.0f}, glm::vec2{1.0f, 1.0f}},
                                             {glm::vec3{1.0f, 0.0f, 0.0f}, glm::vec2{1.0f, 0.0f}}}};

const std::array<Vertex, 4> QuadVerticesFull = {{// positions        // texture Coords
                                                 {glm::vec3{-1, 1, 0.0f}, glm::vec2{0.0f, 1.0f}},
                                                 {glm::vec3{-1, -1, 0.0f}, glm::vec2{0.0f, 0.0f}},
                                                 {glm::vec3{1, 1, 0.0f}, glm::vec2{1.0f, 1.0f}},
                                                 {glm::vec3{1, -1, 0.0f}, glm::vec2{1.0f, 0.0f}}}};

const std::array<Vertex, 4> QuadVerticesCentered = {
    {// positions        // texture Coords
     {glm::vec3{-.5f, .5f, 0.0f}, glm::vec2{0.0f, 1.0f}},
     {glm::vec3{-.5f, -.5f, 0.0f}, glm::vec2{0.0f, 0.0f}},
     {glm::vec3{.5f, .5f, 0.0f}, glm::vec2{1.0f, 1.0f}},
     {glm::vec3{.5f, -.5f, 0.0f}, glm::vec2{1.0f, 0.0f}}}};
