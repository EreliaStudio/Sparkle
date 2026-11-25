#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "perlin_world_fixture.hpp"
#include "voxel/biome_generator.hpp"

namespace sparkle_test::biome_validation
{
	enum class Height
	{
		Flat,
		Hills,
		Multilayer,
		BelowZero,
		Tall
	};
	enum class Placement
	{
		Single,
		Blend,
		Islands,
		Tie,
		Checker
	};
	enum class Layers
	{
		Fixed,
		PerlinLow,
		PerlinHigh,
		Weighted,
		Custom,
		ThreeMaterials,
		VariableThickness,
		Independent,
		TwoPerlin,
		DeepPerlin,
		Finite,
		SingleInfinite
	};
	enum class Cut
	{
		None,
		Horizontal,
		Half,
		Staircase
	};

	struct Settings
	{
		Height height = Height::Flat;
		Placement placement = Placement::Single;
		Layers layers = Layers::Fixed;
		float scale = 1.0f;
		bool terrace = false;
	};

	struct View
	{
		int centerX = 0;
		int centerZ = 0;
		int minimumY = 0;
		int maximumY = 2;
		Cut cut = Cut::None;
		int cutHeight = 20;
		spk::Vector3 camera{52, 52, 59};
		spk::Vector3 target{0, 17, 0};
		bool shadows = true;
		bool reverseOrder = false;

		[[nodiscard]] std::vector<perlin_world::Coordinate> coordinates() const
		{
			std::vector<perlin_world::Coordinate> result;
			for (int z = centerZ - 2; z <= centerZ + 2; ++z)
			{
				for (int x = centerX - 2; x <= centerX + 2; ++x)
				{
					for (int y = minimumY; y <= maximumY; ++y)
					{
						result.push_back({x, y, z});
					}
				}
			}
			return result;
		}

		// Only remove upper voxels from the already generated terrain, exposing its
		// real interior. No material or thickness is synthesized by the test.
		[[nodiscard]] bool removes(std::int64_t x, std::int64_t y, std::int64_t z) const
		{
			switch (cut)
			{
			case Cut::Horizontal:
				return y > cutHeight;
			case Cut::Half:
				return x >= 0 && y > cutHeight;
			case Cut::Staircase:
				return y > (x < -12 ? 27 : x < 12 ? 21
												  : 11) &&
					   z >= -24;
			default:
				return false;
			}
		}
	};

	class Biome final : public spk::Voxel::Biome
	{
	private:
		spk::World::HeightMap _map;
		static void _noise(spk::World::HeightMap &map, const char *name, std::uint32_t seed, float frequency, float amplitude)
		{
			spk::Perlin perlin(seed);
			perlin.setFrequency(frequency);
			perlin.setOctaves(1);
			map.noiseLayers().emplace(name, spk::World::HeightMap::NoiseLayer{perlin, -amplitude, amplitude});
		}
		static void _materialPerlin(Stratum &stratum, std::uint32_t seed, float frequency)
		{
			stratum.distribution = Stratum::Distribution::Perlin;
			stratum.materialNoise.setSeed(seed);
			stratum.materialNoise.setFrequency(frequency);
			stratum.materialNoise.setOctaves(1);
		}

	public:
		Biome(Settings settings, std::uint32_t surfaceID, float altitudeOffset, std::uint32_t seed)
		{
			float altitude = 28.0f + altitudeOffset;
			if (settings.height == Height::BelowZero)
			{
				altitude = -9.0f + altitudeOffset;
			}
			if (settings.height == Height::Tall)
			{
				altitude = 37.0f + altitudeOffset;
			}
			_map.setBaseHeight(altitude);
			if (settings.height != Height::Flat)
			{
				_noise(_map, "broad", seed, 0.055f, settings.height == Height::Tall ? 7.0f : 6.0f);
			}
			if (settings.height == Height::Multilayer)
			{
				_noise(_map, "detail", seed + 1, 0.21f, 3.5f);
				_noise(_map, "fine", seed + 2, 0.46f, 1.5f);
			}
			Stratum top;
			top.materials = {{surfaceID, 1}};
			Stratum middle;
			middle.minimumThickness = middle.maximumThickness = 10;
			middle.materials = {{2, 1}};
			Stratum lower;
			lower.minimumThickness = lower.maximumThickness = 7;
			lower.materials = {{4, 1}};
			Stratum deep;
			deep.thicknessMode = Stratum::ThicknessMode::Infinite;
			deep.materials = {{3, 1}};

			switch (settings.layers)
			{
			case Layers::PerlinLow:
			case Layers::PerlinHigh:
			case Layers::Independent:
			case Layers::TwoPerlin:
				middle.materials = {{2, 1}, {3, 1}};
				_materialPerlin(middle, seed + 11, settings.layers == Layers::PerlinHigh ? 0.31f : 0.055f);
				break;
			case Layers::Weighted:
				middle.materials = {{2, 3}, {3, 1}};
				middle.seed = seed + 31;
				break;
			case Layers::Custom:
				middle.materials = {{2, 1}, {4, 1}};
				middle.distribution = Stratum::Distribution::Custom;
				middle.customDistribution = [](const spk::Vector2 &p) {
					return std::sin(p.x * 0.33f + std::sin(p.y * 0.17f) * 2.5f) > 0 ? 0.75f : 0.25f;
				};
				break;
			case Layers::ThreeMaterials:
				middle.materials = {{2, 2}, {4, 1}, {3, 2}};
				_materialPerlin(middle, seed + 11, 0.09f);
				break;
			case Layers::DeepPerlin:
				deep.materials = {{3, 1}, {4, 1}};
				_materialPerlin(deep, seed + 43, 0.07f);
				break;
			default:
				break;
			}
			if (settings.layers == Layers::VariableThickness || settings.layers == Layers::Independent)
			{
				middle.minimumThickness = 1;
				middle.maximumThickness = 14;
				middle.thicknessNoise.setSeed(seed + 71);
				middle.thicknessNoise.setFrequency(0.065f);
			}
			if (settings.layers == Layers::TwoPerlin)
			{
				lower.materials = {{4, 1}, {3, 1}};
				_materialPerlin(lower, seed + 91, 0.13f);
			}
			if (settings.layers == Layers::SingleInfinite)
			{
				deep.materials = {{surfaceID, 1}};
				strata().push_back(deep);
			}
			else
			{
				strata().push_back(top);
				strata().push_back(middle);
				if (settings.layers != Layers::Finite)
				{
					strata().push_back(lower);
					strata().push_back(deep);
				}
			}
		}

		float groundHeight(const spk::Vector2 &position) const override
		{
			return _map.sample(position);
		}
	};

	class Generator final : public spk::Voxel::BiomeGenerator<perlin_world::Chunk, 3>
	{
	private:
		Settings _settings;
		Biome _first;
		Biome _second;
		Biome _third;
		spk::Perlin _regions{913};

		class Terrace final : public spk::World::SurfacePass<SurfaceData>
		{
		public:
			void apply(const spk::Vector2Int &, SurfaceData &data) override
			{
				auto &grid = *data.heights;
				const auto layout = grid.layout();
				for (std::size_t z = 0; z < layout.height; ++z)
				{
					for (std::size_t x = 0; x < layout.width; ++x)
					{
						grid.sample(x, z) = 4.0f * std::floor(grid.sample(x, z) / 4.0f);
					}
				}
			}
		};

	protected:
		void _computeBiomePlacement(const spk::Vector2 &p, Cell &cell) const override
		{
			switch (_settings.placement)
			{
			case Placement::Single:
				cell[0] = {&_first, 1};
				break;
			case Placement::Blend:
			{
				const float a = std::clamp((p.x + 20.0f) / 40.0f, 0.0f, 1.0f);
				const float b = std::clamp((p.y + 20.0f) / 40.0f, 0.0f, 1.0f);
				cell = {{{&_first, (1 - a) * (1 - b)}, {&_second, a * (1 - b)}, {&_third, b}}};
				break;
			}
			case Placement::Islands:
			{
				const float noise = _regions.sample2D(p.x, p.y);
				const float a = std::clamp((noise - 0.40f) / 0.20f, 0.0f, 1.0f);
				cell[0] = {&_first, 1 - a};
				cell[1] = {&_third, a};
				break;
			}
			case Placement::Tie:
				cell[0] = {&_second, 7};
				cell[1] = {&_first, 7};
				break;
			case Placement::Checker:
			{
				const int index = static_cast<int>(std::floor(p.x / 8.0f)) + static_cast<int>(std::floor(p.y / 8.0f));
				cell[0] = {index % 2 == 0 ? &_first : &_second, 1};
				break;
			}
			}
		}

	public:
		Generator(const perlin_world::Registry &registry, Settings settings) :
			BiomeGenerator(registry, settings.scale),
			_settings(settings),
			_first(settings, 1, 0, 107),
			_second(settings, 4, -12, 211),
			_third(settings, 3, -5, 317)
		{
			_regions.setFrequency(0.048f);
			if (settings.terrace)
			{
				addSurfacePass<Terrace>();
			}
		}
	};
}
