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
		using Grid = StrataGrid::Grid;

	protected:
		virtual const InfluenceGrid &_influences(const spk::Vector2Int &coordinate, const TGenerationData &data) = 0;
		virtual Grid::Lease &_gridDestination(const spk::Vector2Int &coordinate, TGenerationData &data) = 0;

		void _applySurface(const spk::Vector2Int &coordinate, TGenerationData &data) override
		{
			const auto &influences = _influences(coordinate, data);
			auto grid = Grid::obtain({influences.configuration().size, 1.0f}, influences.worldOrigin());
			const auto layout = grid->sampleSize();
			for (std::uint32_t z = 0; z < layout.y; ++z)
			{
				for (std::uint32_t x = 0; x < layout.x; ++x)
				{
					const auto *biome = dynamic_cast<const Biome *>(influences.interpolate(grid->localPosition({x, z})).dominant());
					if (biome == nullptr)
					{
						throw spk::Exception("Voxel strata require a dominant voxel biome");
					}
					grid->buildCell(x, z, *biome);
				}
			}
			_gridDestination(coordinate, data) = std::move(grid);
		}
	};
}
