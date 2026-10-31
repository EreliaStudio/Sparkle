#pragma once
#include "voxel/occlusion/result.hpp"
#include "voxel/definition.hpp"

namespace spk::Voxel::Occlusion
{
 class Algorithm final
 {
 public:
  [[nodiscard]] static Result compute(const Definition &source, Shape::Orientation sourceOrientation,
   const Definition &blocker, Shape::Orientation blockerOrientation,
   Shape::Face direction, float tolerance = 1.0e-5f);
 };
}
