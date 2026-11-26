#pragma once

#include "world/voxel/biome_generation_data.hpp"
#include "world/generator.hpp"

namespace spk::World::Voxel
{
	template <typename TChunk, typename TGenerationData>
	class StrataApplicationPass : public World::Generator<TChunk, TGenerationData>::Pass
	{
	protected:
		virtual const StrataGrid &_strataGrid(const World::ChunkCoordinate &coordinate, const TGenerationData &data) = 0;

	public:
		void apply(const World::ChunkCoordinate &coordinate, TGenerationData &data) override
		{
			auto &builder = data.chunk(coordinate).builder;
			const auto &strata = _strataGrid(coordinate, data);
			const auto origin = TChunk::chunkCoordinateToWorld(coordinate);
			const spk::Vector2UInt size{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)};
			if (strata.size() != size)
			{
				throw spk::Exception("Voxel strata grid does not match the chunk footprint");
			}

			for (auto &&[position, instance] : strata)
			{
				if (!instance)
				{
					throw spk::Exception("Missing voxel stratum instance");
				}
				for (std::uint32_t y = 0; y < static_cast<std::uint32_t>(TChunk::Size.y); ++y)
				{
					const auto id = instance->voxelIDAt(static_cast<double>(origin.y + y));
					if (id != 0)
					{
						builder.set({position.x, y, position.y}, id);
					}
				}
			}
		}
	};
}
