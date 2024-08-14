#include "Frustum.hpp"

#include <glm/ext/matrix_float4x4.hpp>

Frustum::Frustum(const glm::mat4& clip_matrix) {
  data_[Left][X] = clip_matrix[0][3] + clip_matrix[0][0];
  data_[Left][Y] = clip_matrix[1][3] + clip_matrix[1][0];
  data_[Left][Z] = clip_matrix[2][3] + clip_matrix[2][0];
  data_[Left][Dist] = clip_matrix[3][3] + clip_matrix[3][0];

  data_[Right][X] = clip_matrix[0][3] - clip_matrix[0][0];
  data_[Right][Y] = clip_matrix[1][3] - clip_matrix[1][0];
  data_[Right][Z] = clip_matrix[2][3] - clip_matrix[2][0];
  data_[Right][Dist] = clip_matrix[3][3] - clip_matrix[3][0];

  data_[Top][X] = clip_matrix[0][3] - clip_matrix[0][1];
  data_[Top][Y] = clip_matrix[1][3] - clip_matrix[1][1];
  data_[Top][Z] = clip_matrix[2][3] - clip_matrix[2][1];
  data_[Top][Dist] = clip_matrix[3][3] - clip_matrix[3][1];

  data_[Bottom][X] = clip_matrix[0][3] + clip_matrix[0][1];
  data_[Bottom][Y] = clip_matrix[1][3] + clip_matrix[1][1];
  data_[Bottom][Z] = clip_matrix[2][3] + clip_matrix[2][1];
  data_[Bottom][Dist] = clip_matrix[3][3] + clip_matrix[3][1];

  data_[Front][X] = clip_matrix[0][3] - clip_matrix[0][2];
  data_[Front][Y] = clip_matrix[1][3] - clip_matrix[1][2];
  data_[Front][Z] = clip_matrix[2][3] - clip_matrix[2][2];
  data_[Front][Dist] = clip_matrix[3][3] - clip_matrix[3][2];

  data_[Back][X] = clip_matrix[0][3] + clip_matrix[0][2];
  data_[Back][Y] = clip_matrix[1][3] + clip_matrix[1][2];
  data_[Back][Z] = clip_matrix[2][3] + clip_matrix[2][2];
  data_[Back][Dist] = clip_matrix[3][3] + clip_matrix[3][2];

  // Normalize
  for (auto& plane : data_) {
    float length = glm::sqrt(plane[X] * plane[X] + plane[Y] * plane[Y] + plane[Z] * plane[Z]);
    plane[X] /= length;
    plane[Y] /= length;
    plane[Z] /= length;
    plane[Dist] /= length;
  }
}
