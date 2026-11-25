#pragma once

#include "world/pass/biome_distribution_pass.hpp"
#include "world/pass/surface_pass.hpp"
#include "world/surface_biome.hpp"

namespace spk::World
{
	template <typename TGenerationData, std::size_t TCapacity>
	class SurfaceBiomeDistributionPass : public SurfacePass<TGenerationData>, protected BiomeDistribution
	{
	public:
		using Grid = SurfaceBiome::Influence::Grid<TCapacity>;
		using Cell = typename Grid::Cell;
		using Configuration = typename Grid::Configuration;

	protected:
		virtual Configuration _gridConfiguration(const spk::Vector2Int &coordinate, const TGenerationData &data) = 0;
		virtual typename Grid::Lease &_gridDestination(const spk::Vector2Int &coordinate, TGenerationData &data) = 0;
		virtual void _computeBiomePlacement(const spk::Vector2 &worldPosition, Cell &destination) = 0;

	public:
		void apply(const spk::Vector2Int &coordinate, TGenerationData &data) override
		{
			const auto config = _gridConfiguration(coordinate, data);
			const spk::Vector2 origin{static_cast<float>(static_cast<double>(coordinate.x) * config.size.x), static_cast<float>(static_cast<double>(coordinate.y) * config.size.y)};
			auto grid = Grid::obtain(config, origin);
			const auto layout = grid->sampleSize();
			for (std::uint32_t z = 0; z < layout.y; ++z)
			{
				for (std::uint32_t x = 0; x < layout.x; ++x)
				{
					auto &cell = grid->sample({x, z});
					_computeBiomePlacement(origin + grid->localPosition({x, z}), cell);
					_normalize(cell.elements);
				}
			}
			_gridDestination(coordinate, data) = std::move(grid);
		}
	};
}
