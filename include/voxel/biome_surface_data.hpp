#pragma once

#include <map>
#include <mutex>
#include <set>
#include <span>
#include <utility>
#include <vector>

#include "voxel/biome.hpp"
#include "voxel/volume.hpp"
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
	class BiomeGenerationData
	{
	public:
		struct ColumnData
		{
			using InfluenceGrid = World::SurfaceBiome::Influence::Grid<TCapacity>;
			typename InfluenceGrid::Lease influences;
			World::HeightMap::Grid::Lease heights;
			StrataGrid::Grid::Lease strata;

		private:
			friend class BiomeGenerationData;
			mutable std::mutex _mutex;
			bool _ready = false;
		};

		struct ChunkData
		{
			Volume::Builder builder;
		};

	private:
		mutable std::mutex _mutex;
		std::map<spk::Vector2Int, ColumnData> _columns;
		std::map<World::ChunkCoordinate, ChunkData> _chunks;

	public:
		void prepare(const World::ChunkCoordinate &coordinate, const spk::Vector3UInt &dimensions, const Definition::Registry &registry)
		{
			Volume::Builder builder(dimensions);
			builder.bind(registry);
			const std::scoped_lock lock(_mutex);
			if (!_chunks.try_emplace(coordinate, ChunkData{std::move(builder)}).second)
			{
				throw spk::Exception("Chunk generation is already active for this coordinate");
			}
		}

		[[nodiscard]] ChunkData &chunk(const World::ChunkCoordinate &coordinate)
		{
			const std::scoped_lock lock(_mutex);
			return _chunks.at(coordinate);
		}

		[[nodiscard]] const ChunkData &chunk(const World::ChunkCoordinate &coordinate) const
		{
			const std::scoped_lock lock(_mutex);
			return _chunks.at(coordinate);
		}

		[[nodiscard]] Volume::Builder &builder(const World::ChunkCoordinate &coordinate)
		{
			return chunk(coordinate).builder;
		}

		// The active chunk protects its column from flush until clean().
		// Build under a per-column lock; failed builds leave no partial data.
		template <typename TBuild>
		void prepareColumn(const World::ChunkCoordinate &coordinate, TBuild &&build)
		{
			ColumnData *column;
			{
				const std::scoped_lock lock(_mutex);
				if (!_chunks.contains(coordinate))
				{
					throw spk::Exception("Chunk generation is not active");
				}
				column = &_columns.try_emplace(spk::Vector2Int{coordinate.x, coordinate.z}).first->second;
			}
			const std::scoped_lock lock(column->_mutex);
			if (!column->_ready)
			{
				ColumnData pending;
				build(pending);
				column->influences = std::move(pending.influences);
				column->heights = std::move(pending.heights);
				column->strata = std::move(pending.strata);
				column->_ready = true;
			}
		}

		// References remain valid until the caller cleans its active chunk.
		[[nodiscard]] const ColumnData &column(const World::ChunkCoordinate &coordinate) const
		{
			const ColumnData *result;
			{
				const std::scoped_lock lock(_mutex);
				if (!_chunks.contains(coordinate))
				{
					throw spk::Exception("Chunk generation is not active");
				}
				result = &_columns.at({coordinate.x, coordinate.z});
			}
			const std::scoped_lock lock(result->_mutex);
			if (!result->_ready)
			{
				throw spk::Exception("Column generation is not complete");
			}
			return *result;
		}

		void clean(const World::ChunkCoordinate &coordinate) noexcept
		{
			const std::scoped_lock lock(_mutex);
			_chunks.erase(coordinate);
		}

		[[nodiscard]] std::size_t activeChunkCount() const
		{
			const std::scoped_lock lock(_mutex);
			return _chunks.size();
		}

		[[nodiscard]] std::size_t columnCount() const
		{
			const std::scoped_lock lock(_mutex);
			return _columns.size();
		}

		void flush()
		{
			const std::scoped_lock lock(_mutex);
			std::set<spk::Vector2Int> activeColumns;
			for (const auto &[coordinate, chunk] : _chunks)
			{
				(void)chunk;
				activeColumns.emplace(coordinate.x, coordinate.z);
			}
			std::erase_if(_columns, [&](const auto &entry) {
				return !activeColumns.contains(entry.first);
			});
		}
	};
}
