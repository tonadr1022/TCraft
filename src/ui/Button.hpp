#pragma once

#include "ui/Constraint.hpp"

class TextureMaterial;

namespace ui {

struct Button {
  ConstraintType width_constraint_type{ConstraintType::Fixed};
  ConstraintType height_constraint_type{ConstraintType::Fixed};
  float x, y;
  float width, height;
  uint32_t layer;
  std::shared_ptr<TextureMaterial> mat_handle{nullptr};

  bool active_;
};

}  // namespace ui
