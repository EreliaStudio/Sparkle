#pragma once

#include "world/pass/surface_biome_distribution_pass.hpp"
#include "world/pass/surface_height_combination_pass.hpp"
#include "world/voxel/biome_generation_data.hpp"
#include "world/voxel/pass/strata_application_pass.hpp"
#include "world/voxel/pass/strata_generation_pass.hpp"

namespace spk::World::Voxel
{
	// Ready-to-use voxel terrain pipeline. Derive to supply the placement strategy.
	// All four standard passes are registered by the constructor. Extend with addPass or insertPass before generating.
	template <typename TChunk, std::size_t TCapacity>
	class Biome::Generator : public spk::World::Generator<TChunk, Biome::GenerationData<TCapacity>>
	{
	public:
		using Data = Biome::GenerationData<TCapacity>;
		using SurfacePass = spk::World::SurfacePass<TChunk, Data>;
		using ColumnData = typename Data::Column;
		using InfluenceGrid = typename ColumnData::InfluenceGrid;
		using Cell = typename InfluenceGrid::Cell;
		using HeightCombination = typename spk::World::SurfaceHeightCombinationPass<TChunk, Data, TCapacity>::Combination;

	private:
		using DistributionBase = spk::World::SurfaceBiomeDistributionPass<TChunk, Data, TCapacity>;
		using CombinationBase = spk::World::SurfaceHeightCombinationPass<TChunk, Data, TCapacity>;
		using StrataBase = Strata::GenerationPass<TChunk, Data, TCapacity>;
		using ApplicationBase = Strata::ApplicationPass<TChunk, Data>;

		class Distribution : public DistributionBase
		{
		private:
			Generator &_generator;

		protected:
			typename InfluenceGrid::Configuration _computeGridConfiguration(const spk::Vector2Int &, const Data &data) override
			{
				return {{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)}, data.world().surfaceScale};
			}
			void _storeInfluenceGrid(const spk::Vector2Int &coordinate, Data &data, typename InfluenceGrid::Lease grid) override
			{
				data.column(coordinate).influences = std::move(grid);
			}
			void _computeBiomeInfluences(const spk::Vector2 &position, Cell &destination) override
			{
				_generator._computeBiomePlacement(position, destination);
			}

		public:
			explicit Distribution(Generator &generator) :
				_generator(generator)
			{
			}
		};

		class Combination : public CombinationBase
		{
		public:
			using CombinationBase::CombinationBase;

		protected:
			const InfluenceGrid &_influenceGrid(const spk::Vector2Int &coordinate, const Data &data) override
			{
				return *data.column(coordinate).influences;
			}
			void _storeHeightMap(const spk::Vector2Int &coordinate, Data &data, spk::World::HeightMap::Grid::Lease grid) override
			{
				data.column(coordinate).heights = std::move(grid);
			}
		};

		class StrataGeneration : public StrataBase
		{
		protected:
			const InfluenceGrid &_influenceGrid(const spk::Vector2Int &coordinate, const Data &data) override
			{
				return *data.column(coordinate).influences;
			}

			const spk::World::HeightMap::Grid &_heightMap(const spk::Vector2Int &coordinate, const Data &data) override
			{
				return *data.column(coordinate).heights;
			}

			void _storeStrataGrid(const spk::Vector2Int &coordinate, Data &data, Strata::Grid::Lease grid) override
			{
				data.column(coordinate).strata = std::move(grid);
			}
		};

		class Application : public ApplicationBase
		{
		protected:
			const Strata::Grid &_strataGrid(const spk::World::ChunkCoordinate &coordinate, const Data &data) override
			{
				return *data.column(coordinate).strata;
			}
		};

	protected:
		// Derived recipes can specialize the sampling scale before submitting work.
		void _setSurfaceScale(float scale)
		{
			(void)InfluenceGrid::sampleSizeFor({{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)}, scale});
			this->_data().setSurfaceScale(scale);
		}

		virtual void _computeBiomePlacement(const spk::Vector2 &worldPosition, Cell &destination) const = 0;

		void _applyPass(typename spk::World::Generator<TChunk, Data>::Pass &pass, const spk::World::ChunkCoordinate &coordinate, Data &data) override
		{
			if (dynamic_cast<SurfacePass *>(&pass) != nullptr && std::as_const(data).column(coordinate).ready())
			{
				return;
			}
			pass.apply(coordinate, data);
		}

		bool _prepareGenerationData(const spk::World::ChunkCoordinate &coordinate, Data &data) override
		{
			data.prepareChunk(coordinate, spk::Vector3UInt{TChunk::Size});
			return true;
		}

		TChunk _instantiateChunk(const spk::World::ChunkCoordinate &coordinate, Data &data) override
		{
			TChunk chunk;
			chunk.setVolume(data.chunk(coordinate).builder.build());
			data.completeChunk(coordinate);
			return chunk;
		}

		void _cleanGenerationData(const spk::World::ChunkCoordinate &coordinate, Data &data) noexcept override
		{
			data.cleanChunk(coordinate);
		}

	public:
		Generator()
		{
			this->template addPass<Distribution>(*this);
			this->template addPass<Combination>(HeightCombination::Weighted);
			this->template addPass<StrataGeneration>();
			this->template addPass<Application>();
		}

		void bind(const spk::Voxel::Definition::Registry *registry)
		{
			this->_data().bind(registry);
		}
	};
}
