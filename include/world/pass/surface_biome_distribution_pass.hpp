#pragma once

#include "world/pass/surface_pass.hpp"
#include "world/surface_biome.hpp"

namespace spk::World
{
	template <typename TChunk, typename TGenerationData, std::size_t TCapacity = 4>
	class SurfaceBiomeDistributionPass : public SurfacePass<TChunk, TGenerationData>
	{
	public:
		using Grid = SurfaceBiome::Influence::Grid<TCapacity>;
		using Cell = typename Grid::Cell;
		using Configuration = typename Grid::Configuration;

	protected:
		virtual Configuration _computeGridConfiguration(const spk::Vector2Int &coordinate, const TGenerationData &data) = 0;
		virtual void _storeInfluenceGrid(const spk::Vector2Int &coordinate, TGenerationData &data, typename Grid::Lease grid) = 0;
		virtual void _computeBiomeInfluences(const spk::Vector2 &worldPosition, Cell &destination) = 0;

		void _applySurface(const spk::Vector2Int &coordinate, TGenerationData &data) override
		{
			const auto config = _computeGridConfiguration(coordinate, data);
			const spk::Vector2 origin{static_cast<float>(static_cast<double>(coordinate.x) * config.size.x), static_cast<float>(static_cast<double>(coordinate.y) * config.size.y)};
			auto grid = Grid::obtain(config, origin);
			const auto layout = grid->sampleSize();
			for (std::uint32_t z = 0; z < layout.y; ++z)
			{
				for (std::uint32_t x = 0; x < layout.x; ++x)
				{
					auto &cell = grid->sample({x, z});
					_computeBiomeInfluences(origin + grid->localPosition({x, z}), cell);
					cell.normalize();
				}
			}
			_storeInfluenceGrid(coordinate, data, std::move(grid));
		}
	};
}
