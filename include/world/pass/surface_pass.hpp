#pragma once

#include "math/vector2.hpp"

namespace spk::World
{
	// A reusable operation on a column's data. It owns no cache or lifecycle.
	template <typename TSurfaceData>
	class SurfacePass
	{
	public:
		virtual ~SurfacePass() = default;
		virtual void apply(const spk::Vector2Int &coordinate, TSurfaceData &data) = 0;
	};
}
