#pragma once

#include <glm/fwd.hpp>

using FrustumData = float[6][4];
struct Frustum {
  explicit Frustum(const glm::mat4& clip_matrix);

  enum Plane { Right, Left, Top, Bottom, Front, Back };
  enum PlaneComponent { X, Y, Z, Dist };

  [[nodiscard]] glm::vec4 GetPlane(Plane plane) const;

  [[nodiscard]] inline FrustumData& GetData() { return data_; }

 private:
  FrustumData data_;
  // float data_[6][4];
};
