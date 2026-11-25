#pragma once

#include "voxel/pass/strata_application_pass.hpp"
#include "voxel/pass/strata_generation_pass.hpp"
#include "world/pass/surface_biome_distribution_pass.hpp"
#include "world/pass/surface_height_combination_pass.hpp"

namespace spk::Voxel
{
	// Ready-to-use voxel terrain pipeline. Derive to supply the placement strategy.
	// Derived generators register additional passes, then Strata and Application, in their desired order.
	template <typename TChunk, std::size_t TCapacity>
	class BiomeGenerator : public World::Generator<TChunk, BiomeGenerationData<TCapacity>>
	{
	public:
		using GenerationData = BiomeGenerationData<TCapacity>;
		using SurfacePass = World::SurfacePass<TChunk, GenerationData>;
		using SurfaceData = typename GenerationData::ColumnData;
		using InfluenceGrid = typename SurfaceData::InfluenceGrid;
		using Cell = typename InfluenceGrid::Cell;
		using HeightCombination = typename World::SurfaceHeightCombinationPass<TChunk, GenerationData, TCapacity>::Combination;

	private:
		using DistributionBase = World::SurfaceBiomeDistributionPass<TChunk, GenerationData, TCapacity>;
		using CombinationBase = World::SurfaceHeightCombinationPass<TChunk, GenerationData, TCapacity>;
		using StrataBase = StrataGenerationPass<TChunk, GenerationData, TCapacity>;
		using ApplicationBase = StrataApplicationPass<TChunk, GenerationData>;

		class Distribution : public DistributionBase
		{
		private:
			BiomeGenerator &_generator;

		protected:
			typename InfluenceGrid::Configuration _gridConfiguration(const spk::Vector2Int &, const GenerationData &) override
			{
				return {{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)}, _generator._scale};
			}
			typename InfluenceGrid::Lease &_gridDestination(const spk::Vector2Int &coordinate, GenerationData &data) override
			{
				return data.column(coordinate).influences;
			}
			void _computeBiomePlacement(const spk::Vector2 &position, Cell &destination) override
			{
				_generator._computeBiomePlacement(position, destination);
			}

		public:
			explicit Distribution(BiomeGenerator &generator) :
				_generator(generator)
			{
			}
		};

		class Combination : public CombinationBase
		{
		public:
			using CombinationBase::CombinationBase;

		protected:
			const InfluenceGrid &_influences(const spk::Vector2Int &coordinate, const GenerationData &data) override
			{
				return *data.column(coordinate).influences;
			}
			World::HeightMap::Grid::Lease &_gridDestination(const spk::Vector2Int &coordinate, GenerationData &data) override
			{
				return data.column(coordinate).heights;
			}
		};

	protected:
		class Strata : public StrataBase
		{
		protected:
			const InfluenceGrid &_influences(const spk::Vector2Int &coordinate, const GenerationData &data) override
			{
				return *data.column(coordinate).influences;
			}
			StrataGrid::Grid::Lease &_gridDestination(const spk::Vector2Int &coordinate, GenerationData &data) override
			{
				return data.column(coordinate).strata;
			}
		};

		class Application : public ApplicationBase
		{
		protected:
			const World::HeightMap::Grid &_heights(const World::ChunkCoordinate &coordinate, const GenerationData &data) override
			{
				return *data.column(coordinate).heights;
			}
			const StrataGrid::Grid &_strata(const World::ChunkCoordinate &coordinate, const GenerationData &data) override
			{
				return *data.column(coordinate).strata;
			}
		};

	private:
		Definition::Registry _registry;
		float _scale;

	protected:
		virtual void _computeBiomePlacement(const spk::Vector2 &worldPosition, Cell &destination) const = 0;

		bool _prepareGenerationData(const World::ChunkCoordinate &coordinate, GenerationData &data) override
		{
			data.prepare(coordinate, spk::Vector3UInt{TChunk::Size}, _registry);
			return true;
		}

		void _applyPass(typename World::Generator<TChunk, GenerationData>::Pass &pass, const World::ChunkCoordinate &coordinate, GenerationData &data) override
		{
			if (dynamic_cast<SurfacePass *>(&pass) == nullptr || data.needsSurfaceGeneration(coordinate))
			{
				pass.apply(coordinate, data);
			}
		}

		TChunk _instantiateChunk(const World::ChunkCoordinate &coordinate, GenerationData &data) override
		{
			TChunk chunk;
			chunk.setVolume(data.builder(coordinate).build());
			data.publishColumn(coordinate);
			return chunk;
		}

		void _cleanGenerationData(const World::ChunkCoordinate &coordinate, GenerationData &data) noexcept override
		{
			data.clean(coordinate);
		}

	public:
		explicit BiomeGenerator(const Definition::Registry &registry, HeightCombination combination, float scale = 1.0f) :
			_registry(registry),
			_scale(scale)
		{
			(void)InfluenceGrid::sampleSizeFor({{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)}, scale});
			this->template addPass<Distribution>(*this);
			this->template addPass<Combination>(combination);
		}
	};
}
