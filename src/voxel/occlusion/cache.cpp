#include "voxel/occlusion/cache.hpp"
#include <functional>
#include <utility>

namespace spk::Voxel::Occlusion
{
 bool Cache::Key::operator==(const Key &o) const noexcept
 {
  return source == o.source && blocker == o.blocker &&
   sourceOrientation.horizontal == o.sourceOrientation.horizontal &&
   sourceOrientation.vertical == o.sourceOrientation.vertical &&
   blockerOrientation.horizontal == o.blockerOrientation.horizontal &&
   blockerOrientation.vertical == o.blockerOrientation.vertical &&
   direction == o.direction && tolerance == o.tolerance;
 }
 std::size_t Cache::Hash::operator()(const Key &key) const
 {
  auto combine = [](std::size_t &seed, std::size_t value) {
   seed ^= value + 0x9e3779b9U + (seed << 6) + (seed >> 2);
  };
  std::size_t seed = std::hash<const void *>{}(&key.source.shape().instance(key.sourceOrientation));
  combine(seed, std::hash<const void *>{}(&key.blocker.shape().instance(key.blockerOrientation)));
  combine(seed, std::hash<unsigned>{}(static_cast<unsigned>(key.direction)));
  combine(seed, std::hash<float>{}(key.tolerance));
  return seed;
 }
 Result Cache::get(const Key &key)
 {
  {
   auto reader = _data.read();
   auto it = reader->entries.find(key);
   if (it != reader->entries.end()) return it->second;
  }
  Result computed = Algorithm::compute(key.source, key.sourceOrientation,
   key.blocker, key.blockerOrientation, key.direction, key.tolerance);
  auto writer = _data.write();
  return writer->entries.try_emplace(key, std::move(computed)).first->second;
 }
 void Cache::clear()
 {
  auto writer = _data.write();
  writer->entries.clear();
 }
}
