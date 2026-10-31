#pragma once
#include <memory>
#include <vector>
#include "voxel/shape.hpp"

namespace spk::Voxel::Occlusion
{
 class Result final
 {
  std::shared_ptr<const std::vector<Shape::Polygon>> _fragments;
 public:
  Result() : _fragments(std::make_shared<const std::vector<Shape::Polygon>>()) {}
  explicit Result(std::vector<Shape::Polygon> fragments) :
   _fragments(std::make_shared<const std::vector<Shape::Polygon>>(std::move(fragments))) {}
  [[nodiscard]] const std::vector<Shape::Polygon> &fragments() const noexcept { return *_fragments; }
  [[nodiscard]] bool empty() const noexcept { return _fragments->empty(); }
 };
}
