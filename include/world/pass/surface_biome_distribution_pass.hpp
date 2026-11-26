#pragma once

#include <functional>

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
		// The concrete pass owns the distribution; its callable must support concurrent sampling.
		using Distribution = std::function<void(const spk::Vector2 &, Cell &)>;

	private:
		const Distribution *_distribution = nullptr;

	protected:
		void bind(const Distribution *distribution) noexcept
		{
			_distribution = distribution;
		}

		virtual Configuration _computeGridConfiguration(const spk::Vector2Int &coordinate, const TGenerationData &data) = 0;
		virtual void _storeInfluenceGrid(const spk::Vector2Int &coordinate, TGenerationData &data, typename Grid::Lease grid) = 0;

		void _applySurface(const spk::Vector2Int &coordinate, TGenerationData &data) override
		{
			if (_distribution == nullptr || !*_distribution)
			{
				throw spk::Exception("Biome distribution pass has no configured influence function");
			}
			const auto config = _computeGridConfiguration(coordinate, data);
			const spk::Vector2 origin{static_cast<float>(static_cast<double>(coordinate.x) * config.size.x), static_cast<float>(static_cast<double>(coordinate.y) * config.size.y)};
			auto grid = Grid::obtain(config, origin);
			const auto layout = grid->sampleSize();
			for (std::uint32_t z = 0; z < layout.y; ++z)
			{
				for (std::uint32_t x = 0; x < layout.x; ++x)
				{
					auto &cell = grid->sample({x, z});
					(*_distribution)(origin + grid->localPosition({x, z}), cell);
					cell.normalize();
				}
			}
			_storeInfluenceGrid(coordinate, data, std::move(grid));
		}
	};
}
