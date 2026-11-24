#pragma once

#include "voxel/pass/strata_application_pass.hpp"
#include "voxel/pass/strata_generation_pass.hpp"
#include "world/pass/surface_biome_distribution_pass.hpp"
#include "world/pass/surface_height_combination_pass.hpp"

namespace spk::Voxel
{
	// Ready-to-use voxel terrain pipeline. Derive to supply the placement strategy.
	// Additional surface passes run after height combination and before publication.
	template <typename TChunk, std::size_t TCapacity>
	class BiomeGenerator : public World::Generator<TChunk, BiomeGenerationData<TCapacity>, Volume::Builder>
	{
	public:
		using GenerationData = BiomeGenerationData<TCapacity>;
		using SurfaceData = typename GenerationData::Surface;
		using InfluenceGrid = typename SurfaceData::InfluenceGrid;
		using Cell = typename InfluenceGrid::Cell;
		using Builder = Volume::Builder;

	private:
		using SurfaceBase = World::SurfaceGenerationPass<TChunk, GenerationData, SurfaceData, Builder>;
		using DistributionBase = World::SurfaceBiomeDistributionPass<SurfaceData, TCapacity>;
		using CombinationBase = World::SurfaceHeightCombinationPass<SurfaceData, TCapacity>;
		using StrataBase = StrataGenerationPass<SurfaceData, TCapacity>;
		using ApplicationBase = StrataApplicationPass<TChunk, GenerationData, Builder>;

		class Distribution : public DistributionBase
		{
		private:
			BiomeGenerator &_generator;

		protected:
			typename InfluenceGrid::Configuration _gridConfiguration(const spk::Vector2Int &, const SurfaceData &) override
			{
				return {{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)}, _generator._scale};
			}
			typename InfluenceGrid::Lease &_gridDestination(SurfaceData &data) override { return data.influences; }
			void _computeBiomePlacement(const spk::Vector2 &position, Cell &destination) override
			{
				_generator._computeBiomePlacement(position, destination);
			}

		public:
			explicit Distribution(BiomeGenerator &generator) : _generator(generator) {}
		};

		class Combination : public CombinationBase
		{
		protected:
			const InfluenceGrid &_influences(const SurfaceData &data) override { return *data.influences; }
			World::HeightMap::Grid::Lease &_gridDestination(SurfaceData &data) override { return data.heights; }
		};

		class Strata : public StrataBase
		{
		protected:
			const InfluenceGrid &_influences(const SurfaceData &data) override { return *data.influences; }
			StrataGrid::Grid::Lease &_gridDestination(SurfaceData &data) override { return data.strata; }
		};

		class Surface : public SurfaceBase
		{
		private:
			Strata _strata;

		protected:
			typename SurfaceBase::Handle &_surfaceDestination(GenerationData &data) override { return data.surface; }
			void _finalizeSurfaceData(const spk::Vector2Int &coordinate, SurfaceData &data) override
			{
				_strata.apply(coordinate, data);
			}
		};

		class Application : public ApplicationBase
		{
		protected:
			const World::HeightMap::Grid &_heights(const GenerationData &data) override { return *data.surface->heights; }
			const StrataGrid::Grid &_strata(const GenerationData &data) override { return *data.surface->strata; }
		};

		Definition::Registry _registry;
		float _scale;
		Surface *_surface;

	protected:
		virtual void _computeBiomePlacement(const spk::Vector2 &worldPosition, Cell &destination) const = 0;

		std::optional<Builder> _createChunkBuilder(const World::ChunkCoordinate &) override
		{
			Builder builder({static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.y),
				static_cast<std::uint32_t>(TChunk::Size.z)});
			builder.bind(_registry);
			return builder;
		}

		GenerationData _createGenerationData() override { return {}; }

	public:
		explicit BiomeGenerator(const Definition::Registry &registry, float scale = 1.0f) :
			_registry(registry), _scale(scale), _surface(&this->template addPass<Surface>())
		{
			(void)InfluenceGrid::layoutFor({{static_cast<std::uint32_t>(TChunk::Size.x), static_cast<std::uint32_t>(TChunk::Size.z)}, scale});
			_surface->template addPass<Distribution>(*this);
			_surface->template addPass<Combination>();
			this->template addPass<Application>();
		}

		template <typename TPass, typename... TArgs>
		TPass &addSurfacePass(TArgs &&...args)
		{
			return _surface->template addPass<TPass>(std::forward<TArgs>(args)...);
		}
	};
}
