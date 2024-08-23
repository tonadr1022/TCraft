#include "Frustum.hpp"

#include <glm/ext/matrix_float4x4.hpp>

Frustum::Frustum(const glm::mat4& clip_matrix) { SetData(clip_matrix); }

void Frustum::SetData(const glm::mat4& clip_matrix) {
  data_[kLeft][kX] = clip_matrix[0][3] + clip_matrix[0][0];
  data_[kLeft][kY] = clip_matrix[1][3] + clip_matrix[1][0];
  data_[kLeft][kZ] = clip_matrix[2][3] + clip_matrix[2][0];
  data_[kLeft][kDist] = clip_matrix[3][3] + clip_matrix[3][0];

  data_[kRight][kX] = clip_matrix[0][3] - clip_matrix[0][0];
  data_[kRight][kY] = clip_matrix[1][3] - clip_matrix[1][0];
  data_[kRight][kZ] = clip_matrix[2][3] - clip_matrix[2][0];
  data_[kRight][kDist] = clip_matrix[3][3] - clip_matrix[3][0];

  data_[kTop][kX] = clip_matrix[0][3] - clip_matrix[0][1];
  data_[kTop][kY] = clip_matrix[1][3] - clip_matrix[1][1];
  data_[kTop][kZ] = clip_matrix[2][3] - clip_matrix[2][1];
  data_[kTop][kDist] = clip_matrix[3][3] - clip_matrix[3][1];

  data_[kBottom][kX] = clip_matrix[0][3] + clip_matrix[0][1];
  data_[kBottom][kY] = clip_matrix[1][3] + clip_matrix[1][1];
  data_[kBottom][kZ] = clip_matrix[2][3] + clip_matrix[2][1];
  data_[kBottom][kDist] = clip_matrix[3][3] + clip_matrix[3][1];

  data_[kFront][kX] = clip_matrix[0][3] - clip_matrix[0][2];
  data_[kFront][kY] = clip_matrix[1][3] - clip_matrix[1][2];
  data_[kFront][kZ] = clip_matrix[2][3] - clip_matrix[2][2];
  data_[kFront][kDist] = clip_matrix[3][3] - clip_matrix[3][2];

  data_[kBack][kX] = clip_matrix[0][3] + clip_matrix[0][2];
  data_[kBack][kY] = clip_matrix[1][3] + clip_matrix[1][2];
  data_[kBack][kZ] = clip_matrix[2][3] + clip_matrix[2][2];
  data_[kBack][kDist] = clip_matrix[3][3] + clip_matrix[3][2];

  // Normalize
  for (auto& plane : data_) {
    float length = glm::sqrt(plane[kX] * plane[kX] + plane[kY] * plane[kY] + plane[kZ] * plane[kZ]);
    plane[kX] /= length;
    plane[kY] /= length;
    plane[kZ] /= length;
    plane[kDist] /= length;
  }
}
