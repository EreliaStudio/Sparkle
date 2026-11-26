#pragma once

#include <algorithm>
#include <cmath>

#include "voxel_world_fixture.hpp"
#include "world/voxel/biome_generator.hpp"

namespace sparkle_test::biome_world
{
	class Biome final : public spk::World::Voxel::Biome
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
			middle.thickness = Stratum::Thickness::Variable{2, 5, spk::Perlin{}};
			std::get<Stratum::Thickness::Variable>(middle.thickness).noise.setSeed(seed + 1);
			std::get<Stratum::Thickness::Variable>(middle.thickness).noise.setFrequency(0.09f);
			middle.materials = {{surface == 4 ? 4u : 2u, 3.0f}, {3u, 1.0f}};
			middle.distribution = Stratum::Distribution::Perlin{};
			std::get<Stratum::Distribution::Perlin>(middle.distribution).noise.setSeed(seed + 2);
			std::get<Stratum::Distribution::Perlin>(middle.distribution).noise.setFrequency(0.12f);
			strata().push_back(middle);
			Stratum deep;
			deep.thickness = Stratum::Thickness::Infinite{};
			deep.materials = {{3, 1.0f}};
			strata().push_back(deep);
		}

		float groundHeight(const spk::Vector2 &position) const override
		{
			return _heightMap.sample(position);
		}
	};

	template <bool THard>
	class Placement
	{
	private:
		Biome _plains{7.0f, 2.0f, 1, 11};
		Biome _desert{4.0f, 2.5f, 4, 29};
		Biome _mountain{24.0f, 6.0f, 3, 47};
		spk::Perlin _boundary{123};

	public:
		void compute(const spk::Vector2 &position, spk::World::SurfaceBiome::Influence::Cell<3> &cell) const
		{
			const float shift = _boundary.sample2D(position.x, position.y, -8.0f, 8.0f);
			const float x = position.x + shift;
			const float z = position.y + shift * 0.5f;
			if constexpr (THard)
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

		Placement()
		{
			_boundary.setFrequency(0.055f);
		}
	};

	template <bool THard>
	class Generator final : public spk::World::Voxel::Biome::Generator<voxel_world::Chunk, 3>
	{
	private:
		using Base = spk::World::Voxel::Biome::Generator<voxel_world::Chunk, 3>;
		using Data = typename Base::Data;
		using Cell = typename Base::Cell;

		class PlacementPass final : public Base::DistributionPass
		{
		private:
			Placement<THard> _placement;
			typename Base::DistributionPass::Distribution _function = [this](const spk::Vector2 &position, Cell &cell) {
				_placement.compute(position, cell);
			};

		public:
			PlacementPass()
			{
				this->bind(&_function);
			}
		};

		class FlattenRoad : public spk::World::SurfacePass<voxel_world::Chunk, Data>
		{
		public:
			void _applySurface(const spk::Vector2Int &coordinate, Data &data) override
			{
				auto &column = data.column(coordinate);
				auto &grid = *column.heights;
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

	public:
		Generator(const voxel_world::Registry &registry, bool edit)
		{
			this->bind(&registry);
			this->template addPass<PlacementPass>();
			this->template addPass<typename Base::CombinationPass>(Base::HeightCombination::Weighted);
			if (edit)
			{
				this->template addPass<FlattenRoad>();
			}
			this->template addPass<typename Base::StrataGeneration>();
			this->template addPass<typename Base::Application>();
		}
	};
}
