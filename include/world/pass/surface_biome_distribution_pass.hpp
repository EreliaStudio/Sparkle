#pragma once

#include "world/pass/biome_distribution_pass.hpp"
#include "world/pass/surface_generation_pass.hpp"
#include "world/surface_biome.hpp"

namespace spk::World
{
	template <typename TSurfaceData, std::size_t TCapacity>
	class SurfaceBiomeDistributionPass : public SurfacePass<TSurfaceData>, protected BiomeDistribution
	{
	public:
		using Grid = SurfaceBiome::Influence::Grid<TCapacity>;
		using Cell = typename Grid::Cell;
		using Configuration = typename Grid::Configuration;

	protected:
		virtual Configuration _gridConfiguration(const spk::Vector2Int &coordinate, const TSurfaceData &data) = 0;
		virtual typename Grid::Lease &_gridDestination(TSurfaceData &data) = 0;
		virtual void _computeBiomePlacement(const spk::Vector2 &worldPosition, Cell &destination) = 0;

	public:
		void apply(const spk::Vector2Int &coordinate, TSurfaceData &data) override
		{
			const auto config = _gridConfiguration(coordinate, data);
			const spk::Vector2 origin{static_cast<float>(static_cast<double>(coordinate.x) * config.size.x), static_cast<float>(static_cast<double>(coordinate.y) * config.size.y)};
			auto grid = Grid::obtain(config, origin);
			const auto layout = grid->layout();
			for (std::size_t z = 0; z < layout.height; ++z)
			{
				for (std::size_t x = 0; x < layout.width; ++x)
				{
					auto &cell = grid->sample(x, z);
					_computeBiomePlacement(origin + grid->localPosition(x, z), cell);
					_normalize(cell);
				}
			}
			_gridDestination(data) = std::move(grid);
		}
	};
}
