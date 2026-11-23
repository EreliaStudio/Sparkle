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

	// This fixture shares the same math implementation as production world generation.
	// Both noise fields are constructed once per generated chunk rather than once
	// per voxel sample. All samples use absolute world coordinates.
	struct GenerationData
	{
		std::uint32_t seed;
		spk::Perlin broad;
		spk::Perlin detail;

		explicit GenerationData(std::uint32_t seedValue) :
			seed(seedValue),
			broad(seedValue),
			detail(seedValue + 17u)
		{
			broad.setOctaves(1);
			broad.setFrequency(0.055f);
			detail.setOctaves(1);
			detail.setFrequency(0.16f);
		}
	};

	[[nodiscard]] inline int height(std::int32_t x, std::int32_t z, const GenerationData &data)
	{
		const float broad = data.broad.sample2D(static_cast<float>(x), static_cast<float>(z), -1.0f, 1.0f);
		const float detail = data.detail.sample2D(static_cast<float>(x), static_cast<float>(z), -1.0f, 1.0f);
		return std::clamp(static_cast<int>(std::floor(8.5f + broad * 9.0f + detail * 3.5f)), 3, 15);
	}

	[[nodiscard]] inline int height(std::int32_t x, std::int32_t z, std::uint32_t seed = Seed)
	{
		return height(x, z, GenerationData(seed));
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

	using GeneratorBase = spk::World::Generator<Chunk, GenerationData, Builder>;

	class TerrainPass final : public GeneratorBase::Pass
	{
	public:
		void apply(Builder &builder, const Coordinate &coordinate, GenerationData &data) override
		{
			for (std::uint32_t z = 0; z < 16; ++z)
			{
				for (std::uint32_t x = 0; x < 16; ++x)
				{
					const auto gx = coordinate.x * 16 + static_cast<std::int32_t>(x);
					const auto gz = coordinate.z * 16 + static_cast<std::int32_t>(z);
					const int top = height(gx, gz, data);
					for (std::uint32_t y = 0; y <= static_cast<std::uint32_t>(top); ++y)
					{
						const auto gy = coordinate.y * 16 + static_cast<std::int32_t>(y);
						if (gy > top)
						{
							break;
						}
						std::uint32_t id = gy == top ? (top <= 5 ? 4u : 1u) : (gy >= top - 3 ? 2u : 3u);
						if (gy == top && top > 5 && gx % 13 == 0 && gz % 11 == 0)
						{
							id = 5; // Slope showcase, with distinct profile texture.
						}
						else if (gy == top && top > 5 && gx % 17 == 0 && gz % 7 == 0)
						{
							id = 6; // Slab showcase.
						}
						else if (gy == top && top > 5 && gx % 19 == 0 && gz % 13 == 0)
						{
							id = 7; // Stair showcase.
						}
						builder.set({x, y, z}, id);
					}
				}
			}
		}
	};

	class Generator final : public GeneratorBase
	{
	private:
		const Registry &_registry;
		std::uint32_t _seed;

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
			return GenerationData(_seed);
		}

	public:
		explicit Generator(const Registry &definitions, std::uint32_t seed = Seed) :
			_registry(definitions),
			_seed(seed)
		{
			addPass<TerrainPass>();
		}
	};
}
