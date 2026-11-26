#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "rendering/engine/skybox.hpp"

#include "voxel_world_fixture.hpp"

namespace sparkle_test::biome_showcase
{
	namespace terrain = voxel_world;
	inline constexpr spk::Vector2UInt ImageSize{1920, 1080};
	enum class Landscape
	{
		Canyon,
		Peaks,
		Plains,
		Sea,
		Cliffs,
		City,
		Dunes,
		Volcano,
		Coast
	};

	enum Material : std::uint32_t
	{
		Grass = 1,
		Soil,
		Rock,
		Sand,
		Terracotta,
		RedRock,
		Ochre,
		Chalk,
		Water,
		Concrete,
		Glass,
		Basalt,
		Snow
	};

	struct Tile
	{
		const char *name;
		std::array<std::uint8_t, 3> color;
		float opacity = 1.0f;
	};

	inline constexpr std::array<Tile, 13> Tiles{{{"grass", {104, 165, 71}}, {"soil", {126, 90, 61}}, {"rock", {132, 139, 143}}, {"sand", {222, 190, 130}}, {"terracotta", {191, 106, 66}}, {"red_rock", {145, 65, 49}}, {"ochre", {226, 158, 84}}, {"chalk", {231, 226, 206}}, {"water", {42, 137, 195}, 0.48f}, {"concrete", {147, 153, 164}}, {"glass", {83, 132, 160}}, {"basalt", {67, 64, 67}}, {"snow", {240, 244, 248}}}};

	inline terrain::Registry registry()
	{
		const auto cube = terrain::shape("cube");
		terrain::Registry::Builder builder;
		for (std::size_t i = 0; i < Tiles.size(); ++i)
		{
			spk::Voxel::Definition::SlotMapping slots;
			for (const auto &slot : cube.slots())
			{
				slots.emplace(slot.name, spk::Voxel::Definition::Slot{Tiles[i].name, Tiles[i].opacity});
			}
			builder.add(static_cast<std::uint32_t>(i + 1), spk::Voxel::Definition(cube, std::move(slots)));
		}
		return builder.build();
	}

	class Atlas final : public spk::Texture
	{
	public:
		Atlas()
		{
			constexpr std::uint32_t TileSize = 16;
			constexpr std::uint32_t Width = TileSize * Tiles.size();
			std::array<std::uint8_t, Width * TileSize * 4> pixels{};
			for (std::uint32_t tile = 0; tile < Tiles.size(); ++tile)
			{
				for (std::uint32_t y = 0; y < TileSize; ++y)
				{
					for (std::uint32_t x = 0; x < TileSize; ++x)
					{
						const bool edge = x == 0 || y == 0 || x == TileSize - 1 || y == TileSize - 1;
						const auto index = (y * Width + tile * TileSize + x) * 4;
						for (std::size_t channel = 0; channel < 3; ++channel)
						{
							const auto color = Tiles[tile].color[channel];
							pixels[index + channel] = static_cast<std::uint8_t>(edge ? color * 3 / 4 : color);
						}
						pixels[index + 3] = 255;
					}
				}
			}
			setPixels(std::span<const std::uint8_t>(pixels), {Width, TileSize}, Format::RGBA);
			validate();
		}
	};

	inline spk::Palette palette(const Atlas &atlas)
	{
		spk::Voxel::Palette::Builder result;
		for (std::uint32_t tile = 0; tile < Tiles.size(); ++tile)
		{
			spk::Material::Builder material;
			material.add("albedo", atlas, spk::Texture::Section::fromPixels(atlas.size(), {tile * 16, 0}, {16, 16}));
			result.setMaterial(Tiles[tile].name, material.build());
		}
		return result.build();
	}

	inline float smooth(float x)
	{
		x = std::clamp(x, 0.0f, 1.0f);
		return x * x * (3.0f - 2.0f * x);
	}

	// One shared atlas for the six standard Skybox materials. Sampling the
	// normalized direction keeps the horizon and cloud pattern coherent at seams.
	class DaylightSky final : public spk::Texture
	{
	public:
		static constexpr std::uint32_t FaceSize = 256;

		DaylightSky()
		{
			constexpr std::uint32_t Width = FaceSize * spk::Skybox::FaceCount;
			std::vector<std::uint8_t> pixels(Width * FaceSize * 4);
			for (std::uint32_t face = 0; face < spk::Skybox::FaceCount; ++face)
			{
				for (std::uint32_t y = 0; y < FaceSize; ++y)
				{
					for (std::uint32_t x = 0; x < FaceSize; ++x)
					{
						const float u = 2.0f * (static_cast<float>(x) + 0.5f) / FaceSize - 1.0f;
						const float v = 1.0f - 2.0f * (static_cast<float>(y) + 0.5f) / FaceSize;
						const std::array<spk::Vector3, 6> directions{{{1, v, -u}, {-1, v, u}, {u, 1, v}, {u, -1, -v}, {-u, v, 1}, {u, v, -1}}};
						const auto &direction = directions[face];
						const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
						const float altitude = direction.y / length;
						const float elevation = smooth(std::max(0.0f, altitude));
						const float below = smooth(std::max(0.0f, -altitude));
						const float cloudField = std::sin(13.0f * direction.x / length + 3.0f * std::sin(11.0f * direction.z / length)) + 0.55f * std::sin(21.0f * direction.z / length - 6.0f * altitude);
						const float cloud = 0.55f * smooth((cloudField - 0.7f) / 0.65f) * smooth(altitude * 5.0f) * (1.0f - smooth((altitude - 0.65f) / 0.35f));
						const std::array<float, 3> horizon{201.0f, 225.0f, 242.0f};
						const std::array<float, 3> zenith{65.0f, 137.0f, 217.0f};
						const std::array<float, 3> nadir{150.0f, 174.0f, 191.0f};
						const auto index = (y * Width + face * FaceSize + x) * 4;
						for (std::size_t channel = 0; channel < horizon.size(); ++channel)
						{
							const float sky = horizon[channel] + (zenith[channel] - horizon[channel]) * elevation + (nadir[channel] - horizon[channel]) * below;
							pixels[index + channel] = static_cast<std::uint8_t>(sky + (250.0f - sky) * cloud);
						}
						pixels[index + 3] = 255;
					}
				}
			}
			setPixels(std::span<const std::uint8_t>(pixels), {Width, FaceSize}, Format::RGBA);
			validate();
		}

		void bind(spk::Skybox &skybox) const
		{
			for (std::uint32_t face = 0; face < spk::Skybox::FaceCount; ++face)
			{
				spk::Material::Builder material;
				material.add("albedo", *this, spk::Texture::Section::fromPixels(size(), {face * FaceSize, 0}, {FaceSize, FaceSize}));
				skybox.setMaterial(static_cast<spk::Skybox::Face>(face), material.build());
			}
		}
	};

	inline float shore(const spk::Vector2 &p)
	{
		return p.x - 3.0f * std::sin(p.y * 0.09f);
	}

	inline float canyonCenter(float z)
	{
		return 8.0f * std::sin(z * 0.07f);
	}

	class LandscapeBiome final : public spk::World::Voxel::Biome
	{
	private:
		Landscape _type;
		spk::Perlin _broad{1103};
		spk::Perlin _detail{2909};

		void _layer(std::uint32_t id, std::uint32_t thickness)
		{
			Stratum stratum;
			stratum.materials = {{id, 1}};
			stratum.thickness = Stratum::Thickness::Fixed{thickness};
			strata().push_back(std::move(stratum));
		}

		void _base(std::uint32_t id)
		{
			Stratum stratum;
			stratum.materials = {{id, 1}};
			stratum.thickness = Stratum::Thickness::Infinite{};
			strata().push_back(std::move(stratum));
		}

	public:
		explicit LandscapeBiome(Landscape type) :
			_type(type)
		{
			_broad.setFrequency(0.045f);
			_broad.setOctaves(1);
			_detail.setFrequency(0.15f);
			_detail.setOctaves(1);
			switch (type)
			{
			case Landscape::Canyon:
				_layer(Terracotta, 2);
				_base(Rock);
				break;
			case Landscape::Peaks:
				_layer(Grass, 1);
				_layer(Soil, 2);
				_base(Rock);
				break;
			case Landscape::Plains:
				_layer(Grass, 1);
				_layer(Soil, 4);
				_base(Rock);
				break;
			case Landscape::Sea:
				_layer(Sand, 3);
				_base(Rock);
				break;
			case Landscape::Cliffs:
				_layer(Grass, 1);
				_base(Chalk);
				break;
			case Landscape::City:
				_layer(Concrete, 1);
				_base(Glass);
				break;
			case Landscape::Dunes:
				_base(Sand);
				break;
			default:
				_layer(Basalt, 3);
				_base(Rock);
				break;
			}
		}

		float groundHeight(const spk::Vector2 &p) const override
		{
			const float broad = static_cast<float>(_broad.sample2D(p.x, p.y, -1.0f, 1.0f));
			const float detail = static_cast<float>(_detail.sample2D(p.x, p.y, -1.0f, 1.0f));
			switch (_type)
			{
			case Landscape::Canyon:
				return 4.0f + 37.0f * smooth((std::abs(p.x - canyonCenter(p.y)) - 5.0f) / 13.0f) + 2.0f * broad;
			case Landscape::Peaks:
			{
				// Overlapping, jittered mountain masses with broad foothills and irregular ridges.
				const float u = p.x + 7.0f * broad;
				const float v = p.y + 7.0f * detail;
				constexpr float spacing = 40.0f;
				const auto cx = static_cast<std::int32_t>(std::floor(u / spacing));
				const auto cz = static_cast<std::int32_t>(std::floor(v / spacing));
				float massif = 0.0f;
				for (int z = -1; z <= 1; ++z)
				{
					for (int x = -1; x <= 1; ++x)
					{
						const auto hash = terrain::hash(cx + x, cz + z, 3109);
						const float centerX = (cx + x + 0.5f) * spacing + static_cast<float>(hash % 17) - 8.0f;
						const float centerZ = (cz + z + 0.5f) * spacing + static_cast<float>((hash >> 5) % 17) - 8.0f;
						const float dx = u - centerX;
						const float dz = v - centerZ;
						const float angle = static_cast<float>((hash >> 10) % 16) * 0.19634954f;
						const float along = dx * std::cos(angle) + dz * std::sin(angle);
						const float across = -dx * std::sin(angle) + dz * std::cos(angle);
						const float radiusX = 29.0f + static_cast<float>((hash >> 14) % 9);
						const float radiusZ = 23.0f + static_cast<float>((hash >> 18) % 10);
						const float distance = std::sqrt((along / radiusX) * (along / radiusX) + (across / radiusZ) * (across / radiusZ));
						const float height = 31.0f + static_cast<float>((hash >> 23) % 15);
						const float slope = 1.0f - smooth(distance);
						massif = std::max(massif, height * slope);
					}
				}
				const float ridge = 1.0f - std::abs(detail);
				return std::clamp(7.0f + 4.0f * broad + massif * (0.85f + 0.15f * ridge) + 2.5f * detail, 2.0f, 59.0f);
			}
			case Landscape::Plains:
				return 8.0f + 5.0f * broad + 0.8f * detail;
			case Landscape::Sea:
				return -5.0f + 4.0f * smooth((shore(p) + 16.0f) / 16.0f) + broad;
			case Landscape::Cliffs:
				return 2.0f + 32.0f * smooth((shore(p) - 4.0f) / 3.0f) + 1.5f * broad;
			case Landscape::City:
			{
				const float cx = std::floor((p.x + 9.0f) / 18.0f);
				const float cz = std::floor((p.y + 9.0f) / 18.0f);
				const auto hash = terrain::hash(static_cast<std::int32_t>(cx), static_cast<std::int32_t>(cz), 571);
				const float dx = std::abs(p.x - cx * 18.0f);
				const float dz = std::abs(p.y - cz * 18.0f);
				return dx < 5.0f && dz < 5.0f ? 16.0f + static_cast<float>(hash % 39) : 2.0f;
			}
			case Landscape::Dunes:
				return 9.0f + 6.0f * std::sin(p.x * 0.22f + 1.4f * std::sin(p.y * 0.06f)) + 2.0f * broad;
			default:
			{
				const float distance = std::sqrt(p.x * p.x + p.y * p.y);
				return 3.0f + 47.0f * (1.0f - smooth(distance / 39.0f)) - 25.0f * (1.0f - smooth(distance / 9.0f)) + 2.0f * broad;
			}
			}
		}
	};

	class Generator final : public spk::World::Voxel::Biome::Generator<terrain::Chunk, 2>
	{
	private:
		using Base = spk::World::Voxel::Biome::Generator<terrain::Chunk, 2>;
		using BiomePointer = const spk::World::SurfaceBiome *;

		class FixedDistribution final : public Base::DistributionPass<BiomePointer>
		{
		protected:
			BiomePointer _prepareDistribution(const spk::World::Biome::Registry &registry) const override
			{
				return dynamic_cast<BiomePointer>(registry.get("landscape"));
			}

			void _distribute(const spk::Vector2 &, const BiomePointer &biome, Cell &cell) const override
			{
				cell[0] = {biome, 1.0f};
			}
		};

		using CoastData = std::array<BiomePointer, 2>;
		class CoastDistribution final : public Base::DistributionPass<CoastData>
		{
		protected:
			CoastData _prepareDistribution(const spk::World::Biome::Registry &registry) const override
			{
				return {dynamic_cast<BiomePointer>(registry.get("sea")), dynamic_cast<BiomePointer>(registry.get("cliffs"))};
			}

			void _distribute(const spk::Vector2 &p, const CoastData &biomes, Cell &cell) const override
			{
				const float land = smooth((shore(p) + 1.0f) / 3.0f);
				cell[0] = {biomes[0], 1.0f - land};
				cell[1] = {biomes[1], land};
			}
		};

		// Optional application pass: the standard strata pass still resolves all
		// solid terrain. This pass adds sea water or exposes absolute rock bands.
		class DetailPass final : public Base::Pass
		{
		private:
			Landscape _type;

		public:
			explicit DetailPass(Landscape type) :
				_type(type)
			{
			}

			void apply(const terrain::Coordinate &coordinate, Data &data) override
			{
				auto &builder = data.chunk(coordinate).builder;
				const auto &column = std::as_const(data).column(coordinate);
				const auto origin = terrain::Chunk::chunkCoordinateToWorld(coordinate);
				for (auto &&[position, instance] : *column.strata)
				{
					for (std::uint32_t y = 0; y < 16; ++y)
					{
						const auto altitude = origin.y + y;
						const auto id = instance->voxelIDAt(static_cast<double>(altitude));
						if ((_type == Landscape::Sea || _type == Landscape::Coast) && id == 0 && altitude <= 0)
						{
							builder.set({position.x, y, position.y}, Water);
						}
						else if (_type == Landscape::Canyon && id == Rock)
						{
							const int band = static_cast<int>((altitude % 12 + 12) % 12);
							builder.set({position.x, y, position.y}, band < 4 ? RedRock : (band < 8 ? Terracotta : Ochre));
						}
					}
				}
			}
		};

		spk::World::Biome::Registry _biomes;

	public:
		Generator(const terrain::Registry &definitions, Landscape type)
		{
			if (type == Landscape::Coast)
			{
				_biomes.add<LandscapeBiome>("sea", Landscape::Sea);
				_biomes.add<LandscapeBiome>("cliffs", Landscape::Cliffs);
			}
			else
			{
				_biomes.add<LandscapeBiome>("landscape", type);
			}
			bind(&_biomes);
			bind(&definitions);
			if (type == Landscape::Coast)
			{
				addPass<CoastDistribution>();
			}
			else
			{
				addPass<FixedDistribution>();
			}
			addPass<CombinationPass>(HeightCombination::Weighted);
			addPass<StrataGeneration>();
			addPass<Application>();
			if (type == Landscape::Canyon || type == Landscape::Sea || type == Landscape::Coast)
			{
				addPass<DetailPass>(type);
			}
		}
	};
}
