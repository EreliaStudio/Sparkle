#pragma once

#include "world/height_map.hpp"
#include "world/pass/surface_pass.hpp"
#include "world/surface_biome.hpp"

namespace spk::World
{
	template <typename TChunk, typename TGenerationData, std::size_t TCapacity>
	class SurfaceHeightCombinationPass : public SurfacePass<TChunk, TGenerationData>
	{
	public:
		using InfluenceGrid = SurfaceBiome::Influence::Grid<TCapacity>;
		using Grid = HeightMap::Grid;
		enum class Combination
		{
			Weighted,
			Dominant
		};

	private:
		Combination _combination;

	protected:
		virtual const InfluenceGrid &_influences(const spk::Vector2Int &coordinate, const TGenerationData &data) = 0;
		virtual Grid::Lease &_gridDestination(const spk::Vector2Int &coordinate, TGenerationData &data) = 0;

	public:
		explicit SurfaceHeightCombinationPass(Combination combination) :
			_combination(combination)
		{
			if (combination != Combination::Weighted && combination != Combination::Dominant)
			{
				throw spk::Exception("Unknown biome height combination");
			}
		}

	protected:
		void _applySurface(const spk::Vector2Int &coordinate, TGenerationData &data) override
		{
			const auto &influences = _influences(coordinate, data);
			const auto &config = influences.configuration();
			auto grid = Grid::obtain({config.size, config.scale}, influences.worldOrigin());
			const auto layout = grid->sampleSize();
			for (std::uint32_t z = 0; z < layout.y; ++z)
			{
				for (std::uint32_t x = 0; x < layout.x; ++x)
				{
					double height = 0.0;
					bool found = false;
					const auto position = grid->worldOrigin() + grid->localPosition({x, z});
					const auto &cell = influences.sample({x, z});
					if (_combination == Combination::Dominant)
					{
						const auto *winner = cell.dominant();
						if (winner != nullptr)
						{
							height = winner->groundHeight(position);
							found = true;
						}
					}
					else
					{
						for (const auto &influence : cell)
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
					}
					if (!found || !std::isfinite(static_cast<float>(height)))
					{
						throw spk::Exception("Biome height combination has no usable result");
					}
					grid->sample({x, z}) = static_cast<float>(height);
				}
			}
			_gridDestination(coordinate, data) = std::move(grid);
		}
	};
}
