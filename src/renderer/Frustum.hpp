#pragma once

#include <glm/fwd.hpp>

using FrustumData = float[6][4];
struct Frustum {
  Frustum() = default;
  explicit Frustum(const glm::mat4& clip_matrix);
  void SetData(const glm::mat4& clip_matrix);

  enum Plane { kRight, kLeft, kTop, kBottom, kFront, kBack };
  enum PlaneComponent { kX, kY, kZ, kDist };

  [[nodiscard]] glm::vec4 GetPlane(Plane plane) const;

  [[nodiscard]] inline FrustumData& GetData() { return data_; }

 private:
  FrustumData data_;
  // float data_[6][4];
};
