#pragma once

#include <span>

#include "voxel/biome.hpp"
#include "world/height_map.hpp"

namespace spk::Voxel
{
	class StrataGrid
	{
	public:
		struct Cell
		{
			std::size_t offset = 0;
			std::size_t count = 0;
		};

		class Grid : public World::SurfaceGrid<Cell>
		{
		private:
			std::vector<Biome::Layer> _layers;

			explicit Grid(std::size_t capacity) : SurfaceGrid(capacity) {}

		public:
			using Lease = spk::Pool<Grid>::Lease;

			[[nodiscard]] static Lease obtain(const Configuration &configuration, const spk::Vector2 &origin)
			{
				const auto layout = layoutFor(configuration);
				static spk::CapacityPool<Grid> pool([](std::size_t capacity) { return new Grid(capacity); });
				return pool.obtain(layout.count(), [&](Grid &grid) {
					grid._configure(configuration, origin, layout);
					grid._layers.clear();
				});
			}

			void buildCell(std::size_t x, std::size_t z, const Biome &biome)
			{
				const auto offset = _layers.size();
				biome.sampleStrata(worldOrigin() + localPosition(x, z), [&](Biome::Layer layer) {
					_layers.push_back(layer);
				});
				sample(x, z) = {offset, _layers.size() - offset};
			}

			[[nodiscard]] std::span<const Biome::Layer> layers(const spk::Vector2 &position) const
			{
				const auto &cell = at(position);
				return std::span<const Biome::Layer>(_layers).subspan(cell.offset, cell.count);
			}
		};
	};

	template <std::size_t TCapacity>
	struct BiomeSurfaceData
	{
		using InfluenceGrid = World::SurfaceBiome::Influence::Grid<TCapacity>;
		typename InfluenceGrid::Lease influences;
		World::HeightMap::Grid::Lease heights;
		StrataGrid::Grid::Lease strata;
	};

	template <std::size_t TCapacity>
	struct BiomeGenerationData
	{
		using Surface = BiomeSurfaceData<TCapacity>;
		std::shared_ptr<const Surface> surface;
	};
}
