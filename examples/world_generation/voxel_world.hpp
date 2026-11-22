#pragma once

#include <cstdint>
#include <optional>

#include "voxel/chunk.hpp"
#include "world/generator.hpp"

namespace sparkle_world_generation_example
{
	using Chunk = spk::Voxel::Chunk<8, 16, 32>;
	using Registry = spk::Voxel::Definition::Registry;

	[[nodiscard]] inline Registry makeDefinitions()
	{
		spk::Voxel::Shape::Builder shapeBuilder;
		shapeBuilder.addSlot("surface");
		const spk::Voxel::Shape shape = shapeBuilder.build();

		Registry::Builder builder;
		builder.add(1, spk::Voxel::Definition(shape, {{"surface", {"ground"}}}));
		builder.add(2, spk::Voxel::Definition(shape, {{"surface", {"wall"}}}));
		return builder.build();
	}

	struct GenerationData
	{
		const Registry &definitions;
		std::int64_t seed;
	};

	using Generator = spk::Generator<Chunk, GenerationData, spk::Voxel::Volume::Builder>;

	class TerrainPass final : public Generator::Pass
	{
	public:
		void apply(Generator::Builder &volume, const spk::ChunkCoordinate &coordinate, GenerationData &data) override
		{

			volume.bind(data.definitions);
			const auto origin = Chunk::chunkCoordinateToWorld(coordinate);
			for (std::uint32_t z = 0; z < static_cast<std::uint32_t>(Chunk::Size.z); ++z)
			{
				for (std::uint32_t x = 0; x < static_cast<std::uint32_t>(Chunk::Size.x); ++x)
				{
					const std::int64_t value = (origin.x + x) * 13 + (origin.z + z) * 7 + data.seed;
					const std::int64_t height = (value % 3 + 3) % 3;
					for (std::uint32_t y = 0; y < static_cast<std::uint32_t>(Chunk::Size.y); ++y)
					{
						if (origin.y + y <= height)
						{
							volume.set({x, y, z}, 1);
						}
					}
				}
			}
		}
	};

	class WallPass final : public Generator::Pass
	{
	public:
		void apply(Generator::Builder &volume, const spk::ChunkCoordinate &coordinate, GenerationData &) override
		{

			for (std::int64_t x = -2; x <= 10; ++x)
			{
				for (std::int64_t y = 3; y <= 5; ++y)
				{
					const spk::ChunkWorldCoordinate world{x, y, 5};
					if (Chunk::worldToChunkCoordinate(world) != coordinate)
					{
						continue;
					}
					volume.set(spk::Vector3UInt(Chunk::worldToLocalCoordinate(world)), 2);
				}
			}
		}
	};

	class WorldGenerator final : public Generator
	{
	private:
		Registry _definitions;
		std::int64_t _seed;

		[[nodiscard]] std::optional<Generator::Builder> _createChunkBuilder(const spk::ChunkCoordinate &coordinate) override
		{
			if (coordinate.x < -1 || coordinate.x > 1 || coordinate.y != 0 || coordinate.z != 0)
			{
				return std::nullopt;
			}
			return Generator::Builder(spk::Vector3UInt{Chunk::Size});
		}

		[[nodiscard]] GenerationData _createGenerationData() override
		{
			return {_definitions, _seed};
		}

	public:
		explicit WorldGenerator(std::int64_t seed) :
			_definitions(makeDefinitions()),
			_seed(seed)
		{
			addPass<TerrainPass>();
			addPass<WallPass>();
		}
	};
}
