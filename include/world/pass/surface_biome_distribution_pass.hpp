#pragma once

#include <cstdint>

#include "world/pass/surface_pass.hpp"
#include "world/surface_biome.hpp"

namespace spk::World
{
	template <typename TChunk, typename TGenerationData, typename TDistributionData, std::size_t TCapacity = 4>
	class SurfaceBiomeDistributionPass : public SurfacePass<TChunk, TGenerationData>
	{
	public:
		using Grid = SurfaceBiome::Influence::Grid<TCapacity>;
		using Cell = typename Grid::Cell;
		using Configuration = typename Grid::Configuration;

	private:
		Configuration _configuration{{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)}, 1.0f};

	protected:
		// Configure in the concrete pass before submitting generation work.
		void setScale(float scale)
		{
			auto configuration = _configuration;
			configuration.scale = scale;
			(void)Grid::sampleSizeFor(configuration);
			_configuration = configuration;
		}

		// Prepare local state once per column. Both hooks must support concurrent calls.
		[[nodiscard]] virtual TDistributionData _prepareDistribution(const Biome::Registry &registry) const = 0;
		virtual void _distribute(const spk::Vector2 &position, const TDistributionData &distributionData, Cell &cell) const = 0;

		virtual const Biome::Registry *_biomeRegistry(const TGenerationData &data) const = 0;
		virtual void _storeInfluenceGrid(const spk::Vector2Int &coordinate, TGenerationData &data, typename Grid::Lease grid) = 0;

		void _applySurface(const spk::Vector2Int &coordinate, TGenerationData &data) override
		{
			const auto *registry = _biomeRegistry(data);
			if (registry == nullptr)
			{
				throw spk::Exception("Biome distribution pass has no bound biome registry");
			}
			const auto distributionData = _prepareDistribution(*registry);
			const auto &config = _configuration;
			const spk::Vector2 origin{static_cast<float>(static_cast<double>(coordinate.x) * config.size.x), static_cast<float>(static_cast<double>(coordinate.y) * config.size.y)};
			auto grid = Grid::obtain(config, origin);
			const auto layout = grid->sampleSize();
			for (std::uint32_t z = 0; z < layout.y; ++z)
			{
				for (std::uint32_t x = 0; x < layout.x; ++x)
				{
					auto &cell = grid->sample({x, z});
					_distribute(origin + grid->localPosition({x, z}), distributionData, cell);
					cell.normalize();
				}
			}
			_storeInfluenceGrid(coordinate, data, std::move(grid));
		}
	};
}
