#pragma once

#include <span>
#include <vector>

#include "voxel/biome.hpp"
#include "voxel/volume.hpp"
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

			Grid() = default;

		public:
			using Lease = spk::Pool<Grid>::Lease;

			[[nodiscard]] static Lease obtain(const Configuration &configuration, const spk::Vector2 &origin)
			{
				return _obtain<Grid>(configuration, origin, []() {
					return new Grid();
				}, [](Grid &grid) {
					grid._layers.clear();
				});
			}

			void buildCell(std::size_t x, std::size_t z, const Biome &biome)
			{
				const auto dimensions = sampleSize();
				if (x >= dimensions.x || z >= dimensions.y)
				{
					throw spk::Exception("Surface grid sample index is outside its domain");
				}
				const spk::Vector2UInt position{static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(z)};
				const auto offset = _layers.size();
				biome.sampleStrata(worldOrigin() + localPosition(position), [&](Biome::Layer layer) {
					_layers.push_back(layer);
				});
				sample(position) = {offset, _layers.size() - offset};
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
		Volume::Builder builder;
		std::shared_ptr<const Surface> surface;
	};
}
