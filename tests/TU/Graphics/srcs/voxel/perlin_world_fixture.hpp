#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <utility>

#include "container/json/object.hpp"
#include "graphics/opengl/texture.hpp"
#include "math/perlin.hpp"
#include "voxel/pass/perlin_terrain_pass.hpp"
#include "voxel/chunk.hpp"
#include "voxel/definition.hpp"
#include "voxel/palette.hpp"
#include "world/generator.hpp"

namespace sparkle_test::perlin_world
{
	using Chunk = spk::Voxel::Chunk<16, 16, 16>;
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

	// Perlin is configured by the production pass, not implemented by this fixture.
	struct GenerationData
	{
	};

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

	// Keep the test-owned palette/shapes separate from Sparkle's reusable pass.
	using GeneratorBase = spk::World::Generator<Chunk, GenerationData, Builder>;
	using TerrainPass = spk::Voxel::PerlinTerrainPass<Chunk, GenerationData>;

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
		};

	private:
		const Registry &_registry;
		TerrainPass *_terrain = nullptr;

		[[nodiscard]] std::optional<Builder> _createChunkBuilder(const Coordinate &coordinate) override
		{
			if (coordinate.y != 0 || coordinate.x < -4 || coordinate.x > 4 || coordinate.z < -4 || coordinate.z > 4)
			{
				return std::nullopt;
			}
			Builder builder({16, 16, 16});
			builder.bind(_registry);
			return builder;
		}

		[[nodiscard]] GenerationData _createGenerationData() override
		{
			return {};
		}

	public:
		explicit Generator(const Registry &definitions, Settings settings) :
			_registry(definitions)
		{
			auto &terrain = addPass<TerrainPass>();
			_terrain = &terrain;
			terrain.setBaseHeight(settings.baseHeight);
			terrain.setHeightRange(settings.minimumHeight, settings.maximumHeight);
			spk::Perlin broad(settings.seed);
			broad.setOctaves(1);
			broad.setFrequency(settings.broadFrequency);
			terrain.addNoiseLayer(std::move(broad), settings.broadAmplitude);
			spk::Perlin detail(settings.seed + 17u);
			detail.setOctaves(1);
			detail.setFrequency(settings.detailFrequency);
			terrain.addNoiseLayer(std::move(detail), settings.detailAmplitude);
			terrain.setSurfaceVoxel(1);
			terrain.setSubsurfaceLayers({{2, 3}});
			terrain.setDeepVoxel(3);
			// The special voxel silhouettes here are purely demonstration data.
			terrain.setSurfaceSelector([](std::int64_t x, std::int64_t z, std::int32_t height) -> std::uint32_t {
				if (height <= 5)
				{
					return 4;
				}
				if (x % 13 == 0 && z % 11 == 0)
				{
					return 5;
				}
				if (x % 17 == 0 && z % 7 == 0)
				{
					return 6;
				}
				if (x % 19 == 0 && z % 13 == 0)
				{
					return 7;
				}
				return 1;
			});
		}

		explicit Generator(const Registry &definitions, std::uint32_t seed = Seed) :
			Generator(definitions, Settings{.seed = seed})
		{
		}

		[[nodiscard]] std::int32_t height(std::int64_t worldX, std::int64_t worldZ) const
		{
			return _terrain->computeHeight(worldX, worldZ);
		}
	};
}
