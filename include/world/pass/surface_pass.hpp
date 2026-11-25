#pragma once

#include "math/vector2.hpp"
#include "world/generator.hpp"

namespace spk::World
{
	// A regular generator pass adapting XYZ coordinates to X/Z.
	template <typename TChunk, typename TGenerationData>
	class SurfacePass : public Generator<TChunk, TGenerationData>::Pass
	{
	protected:
		virtual void _applySurface(const spk::Vector2Int &coordinate, TGenerationData &data) = 0;

	public:
		void apply(const ChunkCoordinate &coordinate, TGenerationData &data) override
		{
			_applySurface({coordinate.x, coordinate.z}, data);
		}
	};
}
