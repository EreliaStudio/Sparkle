#pragma once

#include <utility>

#include "math/vector2.hpp"
#include "world/generator.hpp"

namespace spk::World
{
	// Adapts XYZ coordinates to X/Z; generation data owns the column cache.
	template <typename TChunk, typename TGenerationData>
	class SurfacePass : public Generator<TChunk, TGenerationData>::Pass
	{
	protected:
		virtual void _applySurface(const spk::Vector2Int &coordinate, TGenerationData &data) = 0;

	public:
		void apply(const ChunkCoordinate &coordinate, TGenerationData &data) override
		{
			if (std::as_const(data).surfaceReady(coordinate))
			{
				return;
			}
			_applySurface({coordinate.x, coordinate.z}, data);
		}
	};
}
