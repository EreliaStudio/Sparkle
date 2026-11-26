#pragma once

#include <cstdint>
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
		// The factory resolves biome references once per column; both callables must support concurrent generation.
		using Distribution = std::function<void(const spk::Vector2 &, Cell &)>;
		using DistributionFactory = std::function<Distribution(const Biome::Registry &)>;

	private:
		const DistributionFactory *_distributionFactory = nullptr;
		Configuration _configuration{{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)}, 1.0f};

	protected:
		void bind(const DistributionFactory *factory) noexcept
		{
			_distributionFactory = factory;
		}

		// Configure in the concrete pass before submitting generation work.
		void setScale(float scale)
		{
			auto configuration = _configuration;
			configuration.scale = scale;
			(void)Grid::sampleSizeFor(configuration);
			_configuration = configuration;
		}

		virtual const Biome::Registry *_biomeRegistry(const TGenerationData &data) const = 0;
		virtual void _storeInfluenceGrid(const spk::Vector2Int &coordinate, TGenerationData &data, typename Grid::Lease grid) = 0;

		void _applySurface(const spk::Vector2Int &coordinate, TGenerationData &data) override
		{
			if (_distributionFactory == nullptr || !*_distributionFactory)
			{
				throw spk::Exception("Biome distribution pass has no configured influence function");
			}
			const auto *registry = _biomeRegistry(data);
			if (registry == nullptr)
			{
				throw spk::Exception("Biome distribution pass has no bound biome registry");
			}
			const auto distribution = (*_distributionFactory)(*registry);
			if (!distribution)
			{
				throw spk::Exception("Biome distribution factory returned no influence function");
			}
			const auto &config = _configuration;
			const spk::Vector2 origin{static_cast<float>(static_cast<double>(coordinate.x) * config.size.x), static_cast<float>(static_cast<double>(coordinate.y) * config.size.y)};
			auto grid = Grid::obtain(config, origin);
			const auto layout = grid->sampleSize();
			for (std::uint32_t z = 0; z < layout.y; ++z)
			{
				for (std::uint32_t x = 0; x < layout.x; ++x)
				{
					auto &cell = grid->sample({x, z});
					distribution(origin + grid->localPosition({x, z}), cell);
					cell.normalize();
				}
			}
			_storeInfluenceGrid(coordinate, data, std::move(grid));
		}
	};
}
