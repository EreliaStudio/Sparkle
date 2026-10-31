#include "voxel/occlusion/algorithm.hpp"
#include "exception.hpp"

namespace spk::Voxel::Occlusion
{
 Result Algorithm::compute(const Shape &, Shape::Orientation,
  const Shape &, Shape::Orientation, Shape::Face, float)
 {
  throw spk::Exception("Voxel occlusion clipping is not implemented");
 }
}
