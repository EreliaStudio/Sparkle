#pragma once

#include <concepts>
#include <memory>
#include <utility>
#include <vector>

#include "voxel/pass/strata_application_pass.hpp"
#include "voxel/pass/strata_generation_pass.hpp"
#include "world/pass/surface_biome_distribution_pass.hpp"
#include "world/pass/surface_height_combination_pass.hpp"

namespace spk::Voxel
{
	// Ready-to-use voxel terrain pipeline. Derive to supply the placement strategy.
	// Additional surface passes run after height combination and before publication.
	template <typename TChunk, std::size_t TCapacity>
	class BiomeGenerator : public World::Generator<TChunk, BiomeGenerationData<TCapacity>>
	{
	public:
		using GenerationData = BiomeGenerationData<TCapacity>;
		using SurfaceData = typename GenerationData::ColumnData;
		using InfluenceGrid = typename SurfaceData::InfluenceGrid;
		using Cell = typename InfluenceGrid::Cell;
		using HeightCombination = typename World::SurfaceHeightCombinationPass<SurfaceData, TCapacity>::Combination;

	private:
		using DistributionBase = World::SurfaceBiomeDistributionPass<SurfaceData, TCapacity>;
		using CombinationBase = World::SurfaceHeightCombinationPass<SurfaceData, TCapacity>;
		using StrataBase = StrataGenerationPass<SurfaceData, TCapacity>;
		using ApplicationBase = StrataApplicationPass<TChunk, GenerationData>;

		class Distribution : public DistributionBase
		{
		private:
			BiomeGenerator &_generator;

		protected:
			typename InfluenceGrid::Configuration _gridConfiguration(const spk::Vector2Int &, const SurfaceData &) override
			{
				return {{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)}, _generator._scale};
			}
			typename InfluenceGrid::Lease &_gridDestination(SurfaceData &data) override
			{
				return data.influences;
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
			const InfluenceGrid &_influences(const SurfaceData &data) override
			{
				return *data.influences;
			}
			World::HeightMap::Grid::Lease &_gridDestination(SurfaceData &data) override
			{
				return data.heights;
			}
		};

		class Strata : public StrataBase
		{
		protected:
			const InfluenceGrid &_influences(const SurfaceData &data) override
			{
				return *data.influences;
			}
			StrataGrid::Grid::Lease &_gridDestination(SurfaceData &data) override
			{
				return data.strata;
			}
		};

		// A regular generator pass. The cache and its synchronization belong to
		// GenerationData, not to the pass or the streaming system.
		class ColumnPass : public World::Generator<TChunk, GenerationData>::Pass
		{
		private:
			using Pass = World::SurfacePass<SurfaceData>;
			std::vector<std::unique_ptr<Pass>> _passes;
			Strata _strata;

		public:
			template <typename TPass, typename... TArgs>
				requires std::derived_from<TPass, Pass>
			TPass &addPass(TArgs &&...args)
			{
				auto pass = std::make_unique<TPass>(std::forward<TArgs>(args)...);
				auto &result = *pass;
				_passes.push_back(std::move(pass));
				return result;
			}

			void apply(const World::ChunkCoordinate &coordinate, GenerationData &data) override
			{
				const spk::Vector2Int key{coordinate.x, coordinate.z};
				data.prepareColumn(coordinate, [&](SurfaceData &column) {
					for (auto &pass : _passes)
					{
						pass->apply(key, column);
					}
					_strata.apply(key, column);
				});
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

		Definition::Registry _registry;
		float _scale;
		ColumnPass *_surface;

	protected:
		virtual void _computeBiomePlacement(const spk::Vector2 &worldPosition, Cell &destination) const = 0;

		bool _prepareGenerationData(const World::ChunkCoordinate &coordinate, GenerationData &data) override
		{
			data.prepare(coordinate, spk::Vector3UInt{TChunk::Size}, _registry);
			return true;
		}

		TChunk _instantiateChunk(const World::ChunkCoordinate &coordinate, GenerationData &data) override
		{
			TChunk chunk;
			chunk.setVolume(data.builder(coordinate).build());
			return chunk;
		}

		void _cleanGenerationData(const World::ChunkCoordinate &coordinate, GenerationData &data) noexcept override
		{
			data.clean(coordinate);
		}

	public:
		explicit BiomeGenerator(const Definition::Registry &registry, HeightCombination combination, float scale = 1.0f) :
			_registry(registry),
			_scale(scale),
			_surface(&this->template addPass<ColumnPass>())
		{
			(void)InfluenceGrid::sampleSizeFor({{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)}, scale});
			_surface->template addPass<Distribution>(*this);
			_surface->template addPass<Combination>(combination);
			this->template addPass<Application>();
		}

		template <typename TPass, typename... TArgs>
		TPass &addSurfacePass(TArgs &&...args)
		{
			return _surface->template addPass<TPass>(std::forward<TArgs>(args)...);
		}
	};
}
