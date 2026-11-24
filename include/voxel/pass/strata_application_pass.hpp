#pragma once

#include "voxel/biome_surface_data.hpp"
#include "world/generator.hpp"

namespace spk::Voxel
{
	template <typename TChunk, typename TGenerationData, typename TBuilder = Volume::Builder>
	class StrataApplicationPass : public World::Generator<TChunk, TGenerationData, TBuilder>::Pass
	{
	protected:
		virtual const World::HeightMap::Grid &_heights(const TGenerationData &data) = 0;
		virtual const StrataGrid::Grid &_strata(const TGenerationData &data) = 0;

	public:
		void apply(TBuilder &builder, const World::ChunkCoordinate &coordinate, TGenerationData &data) override
		{
			const auto &heights = _heights(data);
			const auto &strata = _strata(data);
			const auto origin = TChunk::chunkCoordinateToWorld(coordinate);
			const spk::Vector2UInt size{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)};
			const spk::Vector2 expectedOrigin{static_cast<float>(origin.x), static_cast<float>(origin.z)};
			if (heights.configuration().size != size || strata.configuration().size != size ||
				heights.worldOrigin() != expectedOrigin || strata.worldOrigin() != expectedOrigin)
			{
				throw spk::Exception("Voxel surface grids do not match the chunk footprint");
			}
			for (std::uint32_t z = 0; z < size.y; ++z)
			{
				for (std::uint32_t x = 0; x < size.x; ++x)
				{
					const spk::Vector2 position{static_cast<float>(x), static_cast<float>(z)};
					const double top = std::floor(heights.at(position, World::HeightMap::Grid::Interpolation::Bilinear));
					if (!std::isfinite(top))
					{
						throw spk::Exception("Voxel terrain height must be finite");
					}
					const auto layers = strata.layers(position);
					for (std::uint32_t y = 0; y < static_cast<std::uint32_t>(TChunk::Size.y); ++y)
					{
						double depth = top - static_cast<double>(origin.y + y);
						if (depth < 0.0)
						{
							break;
						}
						for (const auto &layer : layers)
						{
							if (!layer.thickness || depth < *layer.thickness)
							{
								builder.set({x, y, z}, layer.voxelID);
								break;
							}
							depth -= *layer.thickness;
						}
					}
				}
			}
		}
	};
}
