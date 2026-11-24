#pragma once

#include "voxel/biome_surface_data.hpp"
#include "world/pass/surface_generation_pass.hpp"

namespace spk::Voxel
{
	template <typename TSurfaceData, std::size_t TCapacity>
	class StrataGenerationPass : public World::SurfacePass<TSurfaceData>
	{
	public:
		using InfluenceGrid = World::SurfaceBiome::Influence::Grid<TCapacity>;
		using Grid = StrataGrid::Grid;

	protected:
		virtual const InfluenceGrid &_influences(const TSurfaceData &data) = 0;
		virtual Grid::Lease &_gridDestination(TSurfaceData &data) = 0;

	public:
		void apply(const spk::Vector2Int &, TSurfaceData &data) override
		{
			const auto &influences = _influences(data);
			auto grid = Grid::obtain({influences.configuration().size, 1.0f}, influences.worldOrigin());
			const auto layout = grid->layout();
			for (std::size_t z = 0; z < layout.height; ++z)
			{
				for (std::size_t x = 0; x < layout.width; ++x)
				{
					const auto *biome = dynamic_cast<const Biome *>(influences.dominant(grid->localPosition(x, z)));
					if (biome == nullptr)
					{
						throw spk::Exception("Voxel strata require a dominant voxel biome");
					}
					grid->buildCell(x, z, *biome);
				}
			}
			_gridDestination(data) = std::move(grid);
		}
	};
}
