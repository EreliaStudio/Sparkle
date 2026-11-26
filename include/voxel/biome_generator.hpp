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
			typename InfluenceGrid::Configuration _computeGridConfiguration(const spk::Vector2Int &, const GenerationData &data) override
			{
				return {{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)}, data.world().surfaceScale};
			}
			void _storeInfluenceGrid(const spk::Vector2Int &coordinate, GenerationData &data, typename InfluenceGrid::Lease grid) override
			{
				data.column(coordinate).influences = std::move(grid);
			}
			void _computeBiomeInfluences(const spk::Vector2 &position, Cell &destination) override
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
			const InfluenceGrid &_influenceGrid(const spk::Vector2Int &coordinate, const GenerationData &data) override
			{
				return *data.column(coordinate).influences;
			}
			void _storeHeightMap(const spk::Vector2Int &coordinate, GenerationData &data, World::HeightMap::Grid::Lease grid) override
			{
				data.column(coordinate).heights = std::move(grid);
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
			Strata::Grid::Lease &_gridDestination(const spk::Vector2Int &coordinate, GenerationData &data) override
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
			const Strata::Grid &_strata(const World::ChunkCoordinate &coordinate, const GenerationData &data) override
			{
				return *data.column(coordinate).strata;
			}
		};

	protected:
		virtual void _computeBiomePlacement(const spk::Vector2 &worldPosition, Cell &destination) const = 0;

		void _applyPass(typename World::Generator<TChunk, GenerationData>::Pass &pass, const World::ChunkCoordinate &coordinate, GenerationData &data) override
		{
			if (dynamic_cast<SurfacePass *>(&pass) != nullptr && std::as_const(data).column(coordinate).ready())
			{
				return;
			}
			pass.apply(coordinate, data);
		}

		bool _prepareGenerationData(const World::ChunkCoordinate &coordinate, GenerationData &data) override
		{
			data.prepareChunk(coordinate, spk::Vector3UInt{TChunk::Size});
			return true;
		}

		TChunk _instantiateChunk(const World::ChunkCoordinate &coordinate, GenerationData &data) override
		{
			TChunk chunk;
			chunk.setVolume(data.chunk(coordinate).builder.build());
			data.completeChunk(coordinate);
			return chunk;
		}

		void _cleanGenerationData(const World::ChunkCoordinate &coordinate, GenerationData &data) noexcept override
		{
			data.cleanChunk(coordinate);
		}

	public:
		explicit BiomeGenerator(const Definition::Registry &registry, HeightCombination combination, float scale = 1.0f) :
			World::Generator<TChunk, GenerationData>(registry, scale)
		{
			(void)InfluenceGrid::sampleSizeFor({{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)}, scale});
			this->template addPass<Distribution>(*this);
			this->template addPass<Combination>(combination);
		}
	};
}
