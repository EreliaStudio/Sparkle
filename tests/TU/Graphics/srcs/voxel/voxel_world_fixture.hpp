#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <utility>

#include "container/json/object.hpp"
#include "graphics/opengl/texture.hpp"
#include "math/perlin.hpp"
#include "voxel/definition.hpp"
#include "voxel/palette.hpp"
#include "world/voxel/biome_generator.hpp"
#include "world/voxel/chunk.hpp"

namespace sparkle_test::voxel_world
{
	using Chunk = spk::World::Voxel::Chunk<16, 16, 16>;
	using Coordinate = spk::World::ChunkCoordinate;
	using Builder = spk::Voxel::Volume::Builder;
	using Registry = spk::Voxel::Definition::Registry;

	constexpr std::uint32_t Seed = 0xC0FFEE42u;
	constexpr spk::Vector2UInt ImageSize{640, 480};

	// No global PRNG: every sample depends only on the seed and absolute world
	// coordinates, including negative ones. Generation order cannot alter a chunk.
	[[nodiscard]] inline std::uint32_t hash(std::int32_t x, std::int32_t z, std::uint32_t seed) noexcept
	{
		std::uint32_t value = seed ^ (static_cast<std::uint32_t>(x) * 0x9E3779B9u) ^
							  (static_cast<std::uint32_t>(z) * 0x85EBCA6Bu);
		value ^= value >> 16;
		value *= 0x7FEB352Du;
		value ^= value >> 15;
		value *= 0x846CA68Bu;
		return value ^ (value >> 16);
	}

	[[nodiscard]] inline spk::Voxel::Shape shape(const char *name)
	{
		spk::Voxel::Shape::Builder builder;
		builder.addFromJSON(
			spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / (std::string(name) + ".json")));
		return builder.build();
	}

	[[nodiscard]] inline spk::Voxel::Definition definition(const spk::Voxel::Shape &voxelShape, const char *material, const char *profile = nullptr)
	{
		spk::Voxel::Definition::SlotMapping mapping;
		for (const auto &slot : voxelShape.slots())
		{
			mapping.emplace(slot.name, spk::Voxel::Definition::Slot{slot.name == "profile" && profile != nullptr ? profile : material, 1.0f});
		}
		return {voxelShape, std::move(mapping)};
	}

	[[nodiscard]] inline Registry registry()
	{
		const auto cube = shape("cube");
		Registry::Builder builder;
		builder.add(1, definition(cube, "grass"));
		builder.add(2, definition(cube, "dirt"));
		builder.add(3, definition(cube, "stone"));
		builder.add(4, definition(cube, "sand"));
		builder.add(5, definition(shape("slope"), "grass", "dirt"));
		builder.add(6, definition(shape("slab"), "grass", "dirt"));
		builder.add(7, definition(shape("stair"), "stone", "dirt"));
		return builder.build();
	}

	class OutlinedAtlas final : public spk::Texture
	{
	public:
		OutlinedAtlas()
		{
			// Four distinct 16x16 tiles with a two-pixel silhouette on every
			// voxel face. Generated here, not loaded from an external asset.
			constexpr std::array<std::uint8_t, 12> Colors{
				96, 180, 83, 142, 103, 70, 151, 162, 175, 213, 187, 119};
			std::array<std::uint8_t, 64 * 16 * 4> pixels{};
			for (std::uint32_t tile = 0; tile < 4; ++tile)
			{
				for (std::uint32_t y = 0; y < 16; ++y)
				{
					for (std::uint32_t x = 0; x < 16; ++x)
					{
						const bool edge = x < 2 || y < 2 || x >= 14 || y >= 14;
						const auto noise = hash(static_cast<std::int32_t>(x + tile * 16), static_cast<std::int32_t>(y), Seed);
						const auto variation = static_cast<std::uint8_t>(noise % 17);
						const auto index = ((y * 64) + tile * 16 + x) * 4;
						for (std::size_t channel = 0; channel < 3; ++channel)
						{
							const int base = static_cast<int>(Colors[tile * 3 + channel]) + static_cast<int>(variation) - 8;
							pixels[index + channel] = static_cast<std::uint8_t>(edge ? std::max(12, base / 4) : std::clamp(base, 0, 255));
						}
						pixels[index + 3] = 255;
					}
				}
			}
			setPixels(std::span<const std::uint8_t>(pixels), {64, 16}, Format::RGBA);
			validate();
		}
	};

	[[nodiscard]] inline spk::Palette palette(const OutlinedAtlas &atlas)
	{
		spk::Voxel::Palette::Builder result;
		constexpr std::array<const char *, 4> Names{"grass", "dirt", "stone", "sand"};
		for (std::size_t tile = 0; tile < Names.size(); ++tile)
		{
			spk::Material::Builder material;
			material.add("albedo", atlas, spk::Texture::Section::fromPixels(atlas.size(), {static_cast<std::uint32_t>(tile) * 16u, 0}, {16, 16}));
			result.setMaterial(Names[tile], material.build());
		}
		return result.build();
	}

	// The finite Perlin world is a single-biome recipe using the production pipeline.
	using GeneratorBase = spk::World::Voxel::Biome::Generator<Chunk, 1>;
	using GenerationData = GeneratorBase::Data;

	class Generator final : public GeneratorBase
	{
	public:
		struct Settings
		{
			std::uint32_t seed = Seed;
			float broadFrequency = 0.055f;
			float broadAmplitude = 9.0f;
			float detailFrequency = 0.16f;
			float detailAmplitude = 3.5f;
			float baseHeight = 8.5f;
			std::int32_t minimumHeight = 3;
			std::int32_t maximumHeight = 15;
			std::int32_t chunkRadius = 4;
		};

	private:
		class TerrainBiome final : public spk::World::Voxel::Biome
		{
		private:
			Settings _settings;
			spk::Perlin _broad;
			spk::Perlin _detail;

		public:
			explicit TerrainBiome(Settings settings) :
				_settings(settings),
				_broad(settings.seed),
				_detail(settings.seed + 17u)
			{
				_broad.setOctaves(1);
				_broad.setFrequency(settings.broadFrequency);
				_detail.setOctaves(1);
				_detail.setFrequency(settings.detailFrequency);

				Stratum surface;
				surface.materials = {{1, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1}};
				surface.distribution = Stratum::Distribution::Custom{[this](const spk::Vector2 &position) {
					const auto x = static_cast<std::int64_t>(position.x);
					const auto z = static_cast<std::int64_t>(position.y);
					if (groundHeight(position) <= 5)
					{
						return 0.3f;
					}
					if (x % 13 == 0 && z % 11 == 0)
					{
						return 0.5f;
					}
					if (x % 17 == 0 && z % 7 == 0)
					{
						return 0.7f;
					}
					if (x % 19 == 0 && z % 13 == 0)
					{
						return 0.9f;
					}
					return 0.1f;
				}};
				strata().push_back(std::move(surface));

				Stratum soil;
				soil.thickness = Stratum::Thickness::Fixed{3};
				soil.materials = {{2, 1}};
				strata().push_back(std::move(soil));

				Stratum deep;
				deep.thickness = Stratum::Thickness::Infinite{};
				deep.materials = {{3, 1}};
				strata().push_back(std::move(deep));
			}

			[[nodiscard]] float groundHeight(const spk::Vector2 &position) const override
			{
				double elevation = _settings.baseHeight;
				elevation += static_cast<double>(_settings.broadAmplitude) * _broad.sample2D(position.x, position.y, -1.0f, 1.0f);
				elevation += static_cast<double>(_settings.detailAmplitude) * _detail.sample2D(position.x, position.y, -1.0f, 1.0f);
				return static_cast<float>(std::floor(std::clamp(elevation, static_cast<double>(_settings.minimumHeight), static_cast<double>(_settings.maximumHeight))));
			}
		};

		using DistributionData = const spk::World::SurfaceBiome *;

		class TerrainDistribution final : public DistributionPass<DistributionData>
		{
		protected:
			DistributionData _prepareDistribution(const spk::World::Biome::Registry &registry) const override
			{
				const auto *biome = dynamic_cast<const spk::World::SurfaceBiome *>(registry.get("terrain"));
				if (biome == nullptr)
				{
					throw spk::Exception("Missing surface biome");
				}
				return biome;
			}

			void _distribute(const spk::Vector2 &, const DistributionData &biome, Cell &cell) const override
			{
				cell[0] = {biome, 1.0f};
			}
		};

		spk::World::Biome::Registry _biomes;
		std::int32_t _chunkRadius;

	protected:
		[[nodiscard]] bool _prepareGenerationData(const Coordinate &coordinate, GenerationData &data) override
		{
			if (coordinate.y != 0 || coordinate.x < -_chunkRadius || coordinate.x > _chunkRadius || coordinate.z < -_chunkRadius || coordinate.z > _chunkRadius)
			{
				return false;
			}
			return GeneratorBase::_prepareGenerationData(coordinate, data);
		}

	public:
		explicit Generator(const Registry &definitions, Settings settings) :
			_chunkRadius(settings.chunkRadius)
		{
			_biomes.add<TerrainBiome>("terrain", settings);
			bind(&_biomes);
			bind(&definitions);
			addPass<TerrainDistribution>();
			addPass<CombinationPass>(HeightCombination::Dominant);
			addPass<StrataGeneration>();
			addPass<Application>();
		}

		explicit Generator(const Registry &definitions, std::uint32_t seed = Seed) :
			Generator(definitions, Settings{.seed = seed})
		{
		}

		[[nodiscard]] std::int32_t height(std::int64_t worldX, std::int64_t worldZ) const
		{
			const auto *biome = dynamic_cast<const spk::World::SurfaceBiome *>(_biomes.get("terrain"));
			return static_cast<std::int32_t>(biome->groundHeight({static_cast<float>(worldX), static_cast<float>(worldZ)}));
		}
	};
}
