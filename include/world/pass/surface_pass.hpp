#pragma once

#include "math/vector2.hpp"

namespace spk::World
{
	// A reusable X/Z operation on generation data. Storage and lifetime are defined by TGenerationData.
	template <typename TGenerationData>
	class SurfacePass
	{
	public:
		virtual ~SurfacePass() = default;
		virtual void apply(const spk::Vector2Int &coordinate, TGenerationData &data) = 0;
	};
}
