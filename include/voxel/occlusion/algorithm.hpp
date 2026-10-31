#pragma once
#include "voxel/occlusion/result.hpp"

namespace spk::Voxel::Occlusion
{
 class Algorithm final
 {
 public:
  [[nodiscard]] static Result compute(const Shape &source, Shape::Orientation sourceOrientation,
   const Shape &blocker, Shape::Orientation blockerOrientation,
   Shape::Face direction, float tolerance = 1.0e-5f);
 };
}
