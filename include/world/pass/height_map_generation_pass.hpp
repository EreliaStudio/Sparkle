#pragma once

#include "world/generator.hpp"
#include "world/height_map.hpp"

namespace spk::World
{
	template <typename TChunk, typename TGenerationData>
	class HeightMapGenerationPass : public Generator<TChunk, TGenerationData>::Pass
	{
	private:
		const HeightMap *_heightMap = nullptr;

	protected:
		virtual HeightMap::Grid::Configuration _gridConfiguration(const ChunkCoordinate &coordinate, const TGenerationData &data) = 0;
		virtual HeightMap::Grid::Lease &_gridDestination(const ChunkCoordinate &coordinate, TGenerationData &data) = 0;

	public:
		void bind(const HeightMap *heightMap) noexcept
		{
			_heightMap = heightMap;
		}

		void apply(const ChunkCoordinate &coordinate, TGenerationData &data) override
		{
			if (_heightMap == nullptr)
			{
				throw spk::Exception("HeightMapGenerationPass has no bound height map");
			}
			const auto config = _gridConfiguration(coordinate, data);
			const spk::Vector2 origin{static_cast<float>(static_cast<double>(coordinate.x) * config.size.x), static_cast<float>(static_cast<double>(coordinate.z) * config.size.y)};
			_gridDestination(coordinate, data) = _heightMap->generate(config, origin);
		}
	};
}
