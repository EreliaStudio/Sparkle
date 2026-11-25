#pragma once

#include <algorithm>
#include <cmath>

#include "perlin_world_fixture.hpp"
#include "voxel/biome_generator.hpp"

namespace sparkle_test::biome_world
{
	class Biome final : public spk::Voxel::Biome
	{
	private:
		spk::World::HeightMap _heightMap;

	public:
		Biome(float height, float amplitude, std::uint32_t surface, std::uint32_t seed)
		{
			_heightMap.setBaseHeight(height);
			spk::Perlin noise(seed);
			noise.setFrequency(0.07f);
			_heightMap.noiseLayers().emplace("relief", spk::World::HeightMap::NoiseLayer{noise, -amplitude, amplitude});
			Stratum top;
			top.materials = {{surface, 1.0f}};
			strata().push_back(top);
			Stratum middle;
			middle.minimumThickness = 2;
			middle.maximumThickness = 5;
			middle.thicknessNoise.setSeed(seed + 1);
			middle.thicknessNoise.setFrequency(0.09f);
			middle.materials = {{surface == 4 ? 4u : 2u, 3.0f}, {3u, 1.0f}};
			middle.distribution = Stratum::Distribution::Perlin;
			middle.materialNoise.setSeed(seed + 2);
			middle.materialNoise.setFrequency(0.12f);
			strata().push_back(middle);
			Stratum deep;
			deep.thicknessMode = Stratum::ThicknessMode::Infinite;
			deep.materials = {{3, 1.0f}};
			strata().push_back(deep);
		}

		float groundHeight(const spk::Vector2 &position) const override
		{
			return _heightMap.sample(position);
		}
	};

	class Generator final : public spk::Voxel::BiomeGenerator<perlin_world::Chunk, 3>
	{
	private:
		Biome _plains{7.0f, 2.0f, 1, 11};
		Biome _desert{4.0f, 2.5f, 4, 29};
		Biome _mountain{24.0f, 6.0f, 3, 47};
		spk::Perlin _boundary{123};
		bool _hard;

		class FlattenRoad : public spk::World::SurfacePass<GenerationData>
		{
		public:
			void apply(const spk::Vector2Int &coordinate, GenerationData &data) override
			{
				auto &grid = *data.column(coordinate).heights;
				const auto layout = grid.sampleSize();
				for (std::uint32_t z = 0; z < layout.y; ++z)
				{
					for (std::uint32_t x = 0; x < layout.x; ++x)
					{
						const auto position = grid.worldOrigin() + grid.localPosition({x, z});
						const float weight = std::clamp((7.0f - std::abs(position.y)) / 4.0f, 0.0f, 1.0f);
						grid.sample({x, z}) = std::lerp(grid.sample({x, z}), 7.0f, weight);
					}
				}
			}
		};

	protected:
		void _computeBiomePlacement(const spk::Vector2 &position, Cell &cell) const override
		{
			const float shift = _boundary.sample2D(position.x, position.y, -8.0f, 8.0f);
			const float x = position.x + shift;
			const float z = position.y + shift * 0.5f;
			if (_hard)
			{
				const spk::World::SurfaceBiome *biome = x < -8.0f ? &_desert : (z > 3.0f ? &_mountain : &_plains);
				cell[0] = {biome, 1.0f};
				return;
			}
			const float desert = std::clamp((-x + 1.0f) / 18.0f, 0.0f, 1.0f);
			const float mountain = std::clamp((z + 5.0f) / 20.0f, 0.0f, 1.0f);
			cell[0] = {&_desert, desert};
			cell[1] = {&_mountain, (1.0f - desert) * mountain};
			cell[2] = {&_plains, (1.0f - desert) * (1.0f - mountain)};
		}

	public:
		Generator(const perlin_world::Registry &registry, bool hard, bool edit) :
			BiomeGenerator(registry, HeightCombination::Weighted),
			_hard(hard)
		{
			_boundary.setFrequency(0.055f);
			if (edit)
			{
				addSurfacePass<FlattenRoad>();
			}
		}
	};
}
