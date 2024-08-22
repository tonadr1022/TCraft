#pragma once

#include "ui/Constraint.hpp"

namespace ui {

struct Button {
  ConstraintType width_constraint_type{ConstraintType::Fixed};
  ConstraintType height_constraint_type{ConstraintType::Fixed};
  float x, y;
  float width, height;
  uint32_t layer;
  uint32_t mat_handle;

  bool active_;
};

}  // namespace ui
