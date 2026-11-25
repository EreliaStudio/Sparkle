#pragma once

#include "world/height_map.hpp"
#include "world/pass/surface_generation_pass.hpp"
#include "world/surface_biome.hpp"

namespace spk::World
{
	template <typename TSurfaceData, std::size_t TCapacity>
	class SurfaceHeightCombinationPass : public SurfacePass<TSurfaceData>
	{
	public:
		using InfluenceGrid = SurfaceBiome::Influence::Grid<TCapacity>;
		using Grid = HeightMap::Grid;

	protected:
		virtual const InfluenceGrid &_influences(const TSurfaceData &data) = 0;
		virtual Grid::Lease &_gridDestination(TSurfaceData &data) = 0;

	public:
		void apply(const spk::Vector2Int &, TSurfaceData &data) override
		{
			const auto &influences = _influences(data);
			const auto &config = influences.configuration();
			auto grid = Grid::obtain({config.size, config.scale}, influences.worldOrigin());
			const auto layout = grid->sampleSize();
			for (std::uint32_t z = 0; z < layout.y; ++z)
			{
				for (std::uint32_t x = 0; x < layout.x; ++x)
				{
					double height = 0.0;
					bool found = false;
					const auto position = grid->worldOrigin() + grid->localPosition(x, z);
					for (const auto &influence : influences.sample({x, z}))
					{
						if (influence.biome != nullptr && influence.weight > 0.0f)
						{
							const float value = influence.biome->groundHeight(position);
							if (!std::isfinite(value))
							{
								throw spk::Exception("Biome returned a non-finite height");
							}
							height += static_cast<double>(influence.weight) * value;
							found = true;
						}
					}
					if (!found || !std::isfinite(static_cast<float>(height)))
					{
						throw spk::Exception("Biome height combination has no usable result");
					}
					grid->sample({x, z}) = static_cast<float>(height);
				}
			}
			_gridDestination(data) = std::move(grid);
		}
	};
}
