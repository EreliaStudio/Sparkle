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
		spk::Perlin _boundary{123};

	public:
		void compute(const spk::Vector2 &position, spk::World::SurfaceBiome::Influence::Cell<3> &cell, const spk::World::SurfaceBiome *plains, const spk::World::SurfaceBiome *desertBiome, const spk::World::SurfaceBiome *mountainBiome) const
		{
			const float shift = _boundary.sample2D(position.x, position.y, -8.0f, 8.0f);
			const float x = position.x + shift;
			const float z = position.y + shift * 0.5f;
			if constexpr (THard)
			{
				const spk::World::SurfaceBiome *biome = x < -8.0f ? desertBiome : (z > 3.0f ? mountainBiome : plains);
				cell[0] = {biome, 1.0f};
				return;
			}
			const float desert = std::clamp((-x + 1.0f) / 18.0f, 0.0f, 1.0f);
			const float mountain = std::clamp((z + 5.0f) / 20.0f, 0.0f, 1.0f);
			cell[0] = {desertBiome, desert};
			cell[1] = {mountainBiome, (1.0f - desert) * mountain};
			cell[2] = {plains, (1.0f - desert) * (1.0f - mountain)};
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
			typename Base::DistributionPass::DistributionFactory _factory = [this](const spk::World::Biome::Registry &registry) -> typename Base::DistributionPass::Distribution {
				const auto *plains = dynamic_cast<const spk::World::SurfaceBiome *>(registry.get("plains"));
				const auto *desert = dynamic_cast<const spk::World::SurfaceBiome *>(registry.get("desert"));
				const auto *mountain = dynamic_cast<const spk::World::SurfaceBiome *>(registry.get("mountain"));
				if (plains == nullptr || desert == nullptr || mountain == nullptr)
				{
					throw spk::Exception("Missing surface biome");
				}
				return [this, plains, desert, mountain](const spk::Vector2 &position, Cell &cell) {
					_placement.compute(position, cell, plains, desert, mountain);
				};
			};

		public:
			PlacementPass()
			{
				this->bind(&_factory);
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

		spk::World::Biome::Registry _biomes;

	public:
		Generator(const voxel_world::Registry &registry, bool edit)
		{
			_biomes.add<Biome>("plains", 7.0f, 2.0f, 1, 11);
			_biomes.add<Biome>("desert", 4.0f, 2.5f, 4, 29);
			_biomes.add<Biome>("mountain", 24.0f, 6.0f, 3, 47);
			this->bind(&_biomes);
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
