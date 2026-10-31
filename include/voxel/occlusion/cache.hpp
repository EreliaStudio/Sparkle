#pragma once
#include <cstddef>
#include <unordered_map>
#include "container/protected_data.hpp"
#include "voxel/occlusion/algorithm.hpp"

namespace spk::Voxel::Occlusion
{
 class Cache final
 {
 public:
  struct Key
  {
   Shape source;
   Shape blocker;
   Shape::Orientation sourceOrientation{};
   Shape::Orientation blockerOrientation{};
   Shape::Face direction = Shape::Face::PositiveX;
   float tolerance = 1.0e-5f;
   [[nodiscard]] bool operator==(const Key &other) const noexcept;
  };
 private:
  struct Hash
  {
   [[nodiscard]] std::size_t operator()(const Key &key) const;
  };
  struct Data
  {
   std::unordered_map<Key, Result, Hash> entries;
  };
  spk::ProtectedData<Data> _data;
 public:
  [[nodiscard]] Result get(const Key &key);
  void clear();
 };
}
