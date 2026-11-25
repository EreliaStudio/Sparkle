#pragma once

#include <span>
#include <utility>
#include <vector>

#include "voxel/biome.hpp"
#include "voxel/volume.hpp"
#include "world/chunk_generation_data_collection.hpp"
#include "world/column_generation_data_collection.hpp"
#include "world/height_map.hpp"

namespace spk::Voxel
{
	class StrataGrid
	{
	public:
		struct Cell
		{
			std::size_t offset = 0;
			std::size_t count = 0;
		};

		class Grid : public World::SurfaceGrid<Cell>
		{
		private:
			std::vector<Biome::Layer> _layers;

			Grid() = default;

		public:
			using Lease = spk::Pool<Grid>::Lease;

			[[nodiscard]] static Lease obtain(const Configuration &configuration, const spk::Vector2 &origin)
			{
				return _obtain<Grid>(configuration, origin, []() {
					return new Grid();
				},
									 [](Grid &grid) {
										 grid._layers.clear();
									 });
			}

			void buildCell(std::size_t x, std::size_t z, const Biome &biome)
			{
				const auto dimensions = sampleSize();
				if (x >= dimensions.x || z >= dimensions.y)
				{
					throw spk::Exception("Surface grid sample index is outside its domain");
				}
				const spk::Vector2UInt position{static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(z)};
				const auto offset = _layers.size();
				biome.sampleStrata(worldOrigin() + localPosition(position), [&](Biome::Layer layer) {
					_layers.push_back(layer);
				});
				sample(position) = {offset, _layers.size() - offset};
			}

			[[nodiscard]] std::span<const Biome::Layer> layers(const spk::Vector2 &position) const
			{
				const auto &cell = at(position);
				return std::span<const Biome::Layer>(_layers).subspan(cell.offset, cell.count);
			}
		};
	};

	template <std::size_t TCapacity>
	struct BiomeSurfaceData
	{
		using InfluenceGrid = World::SurfaceBiome::Influence::Grid<TCapacity>;
		typename InfluenceGrid::Lease influences;
		World::HeightMap::Grid::Lease heights;
		StrataGrid::Grid::Lease strata;
	};

	template <std::size_t TCapacity>
	class BiomeGenerationData
	{
	public:
		using ColumnData = BiomeSurfaceData<TCapacity>;
		using ColumnHandle = typename World::ColumnGenerationDataCollection<ColumnData>::Handle;

		struct ChunkData
		{
			Volume::Builder builder;
			ColumnHandle column;
		};

	private:
		World::ColumnGenerationDataCollection<ColumnData> _columns;
		World::ChunkGenerationDataCollection<ChunkData> _chunks;

	public:
		void prepare(const World::ChunkCoordinate &coordinate, const spk::Vector3UInt &dimensions, const Definition::Registry &registry)
		{
			Volume::Builder builder(dimensions);
			builder.bind(registry);
			_chunks.emplace(coordinate, ChunkData{std::move(builder), {}});
		}

		[[nodiscard]] ChunkData &chunk(const World::ChunkCoordinate &coordinate)
		{
			return _chunks.at(coordinate);
		}
		[[nodiscard]] const ChunkData &chunk(const World::ChunkCoordinate &coordinate) const
		{
			return _chunks.at(coordinate);
		}
		[[nodiscard]] Volume::Builder &builder(const World::ChunkCoordinate &coordinate)
		{
			return chunk(coordinate).builder;
		}

		void clean(const World::ChunkCoordinate &coordinate) noexcept
		{
			_chunks.erase(coordinate);
		}

		[[nodiscard]] std::size_t activeChunkCount() const
		{
			return _chunks.count();
		}

		template <typename TBuild>
		[[nodiscard]] ColumnHandle obtainColumn(const spk::Vector2Int &coordinate, TBuild &&build)
		{
			return _columns.obtain(coordinate, std::forward<TBuild>(build));
		}

		[[nodiscard]] std::size_t columnCount() const
		{
			return _columns.count();
		}

		void flush()
		{
			_columns.flush();
		}
	};
}
