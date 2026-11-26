#pragma once

#include "voxel/biome_surface_data.hpp"
#include "world/pass/surface_pass.hpp"

namespace spk::Voxel
{
	template <typename TChunk, typename TGenerationData, std::size_t TCapacity>
	class StrataGenerationPass : public World::SurfacePass<TChunk, TGenerationData>
	{
	public:
		using InfluenceGrid = World::SurfaceBiome::Influence::Grid<TCapacity>;
		using Grid = StrataGrid;

	protected:
		virtual const InfluenceGrid &_influenceGrid(const spk::Vector2Int &coordinate, const TGenerationData &data) = 0;
		virtual const World::HeightMap::Grid &_heightMap(const spk::Vector2Int &coordinate, const TGenerationData &data) = 0;
		virtual void _storeStrataGrid(const spk::Vector2Int &coordinate, TGenerationData &data, Grid::Lease grid) = 0;

		void _applySurface(const spk::Vector2Int &coordinate, TGenerationData &data) override
		{
			const auto &influences = _influenceGrid(coordinate, data);
			const auto &heights = _heightMap(coordinate, data);
			const spk::Vector2UInt size{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)};
			const auto origin = TChunk::chunkCoordinateToWorld({coordinate.x, 0, coordinate.y});
			const spk::Vector2 worldOrigin{static_cast<float>(origin.x), static_cast<float>(origin.z)};
			if (influences.configuration().size != size || heights.configuration().size != size ||
				influences.worldOrigin() != worldOrigin || heights.worldOrigin() != worldOrigin)
			{
				throw spk::Exception("Voxel surface grids do not match the chunk footprint");
			}

			auto grid = Grid::obtain(size);
			for (auto &&[position, instance] : *grid)
			{
				const spk::Vector2 localPosition{static_cast<float>(position.x), static_cast<float>(position.y)};
				const auto *biome = dynamic_cast<const Biome *>(influences.interpolate(localPosition).dominant());
				if (biome == nullptr)
				{
					throw spk::Exception("Voxel strata require a dominant voxel biome");
				}
				instance = Biome::Stratum::Instance::obtain(*biome, worldOrigin + localPosition, heights.interpolate(localPosition));
			}
			_storeStrataGrid(coordinate, data, std::move(grid));
		}
	};
}
