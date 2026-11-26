#pragma once

#include <algorithm>
#include <map>
#include <mutex>
#include <set>
#include <utility>

#include "container/grid_2d.hpp"
#include "voxel/biome.hpp"
#include "voxel/volume.hpp"
#include "world/chunk_coordinate.hpp"
#include "world/height_map.hpp"

namespace spk::Voxel
{
	using StrataGrid = spk::Grid2D<Biome::Stratum::Instance::Lease>;

	template <std::size_t TCapacity = 4>
	class BiomeGenerationData
	{
	public:
		struct WorldData
		{
			Definition::Registry registry;
			float surfaceScale = 1.0f;
		};

		struct ColumnData
		{
			using InfluenceGrid = World::SurfaceBiome::Influence::Grid<TCapacity>;
			typename InfluenceGrid::Lease influences;
			World::HeightMap::Grid::Lease heights;
			StrataGrid::Lease strata;

		private:
			friend class BiomeGenerationData;
			mutable std::mutex _mutex;
			bool _ready = false;

		public:
			// Read while the calling chunk is active, after prepareChunk().
			[[nodiscard]] bool ready() const noexcept
			{
				return _ready;
			}
		};

		struct ChunkData
		{
			Volume::Builder builder;

		private:
			friend class BiomeGenerationData;
			std::unique_lock<std::mutex> _columnLock;

			explicit ChunkData(Volume::Builder value) :
				builder(std::move(value))
			{
			}

		public:
			ChunkData(ChunkData &&) = default;
			ChunkData &operator=(ChunkData &&) = default;
		};

	private:
		using ColumnCollection = std::map<spk::Vector2Int, ColumnData>;
		using ChunkCollection = std::map<World::ChunkCoordinate, ChunkData>;

		mutable std::mutex _mutex;
		WorldData _world;
		ColumnCollection _columns;
		ChunkCollection _chunks;

	public:
		explicit BiomeGenerationData(const Definition::Registry &registry = {}, float surfaceScale = 1.0f) :
			_world{registry, surfaceScale}
		{
		}

		[[nodiscard]] const WorldData &world() const noexcept
		{
			return _world;
		}

		void prepareChunk(const World::ChunkCoordinate &coordinate, const spk::Vector3UInt &dimensions)
		{
			Volume::Builder builder(dimensions);
			builder.bind(_world.registry);
			ColumnData *columnData;
			{
				const std::scoped_lock lock(_mutex);
				if (_chunks.contains(coordinate))
				{
					throw spk::Exception("Chunk generation is already active for this coordinate");
				}
				columnData = &_columns.try_emplace(spk::Vector2Int{coordinate.x, coordinate.z}).first->second;
				_chunks.try_emplace(coordinate, ChunkData(std::move(builder)));
			}
			try
			{
				// Waiting jobs are registered before waiting, so flush cannot evict this column.
				std::unique_lock lock(columnData->_mutex);
				if (!columnData->_ready)
				{
					chunk(coordinate)._columnLock = std::move(lock);
				}
			} catch (...)
			{
				cleanChunk(coordinate);
				throw;
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

		// The calling surface pass must own the column lock.
		[[nodiscard]] ColumnData &column(const spk::Vector2Int &coordinate)
		{
			const std::scoped_lock lock(_mutex);
			auto &columnData = _columns.at(coordinate);
			if (columnData._ready)
			{
				throw spk::Exception("Published column cannot be modified");
			}
			return columnData;
		}

		[[nodiscard]] const ColumnData &column(const spk::Vector2Int &coordinate) const
		{
			const std::scoped_lock lock(_mutex);
			return _columns.at(coordinate);
		}

		// References remain valid until the caller cleans its active chunk.
		[[nodiscard]] ColumnData &column(const World::ChunkCoordinate &coordinate)
		{
			const std::scoped_lock lock(_mutex);
			const auto it = _chunks.find(coordinate);
			if (it == _chunks.end())
			{
				throw spk::Exception("Chunk generation is not active");
			}
			auto &columnData = _columns.at({coordinate.x, coordinate.z});
			if (columnData._ready || !it->second._columnLock.owns_lock())
			{
				throw spk::Exception("Chunk does not own an unpublished column");
			}
			return columnData;
		}

		[[nodiscard]] const ColumnData &column(const World::ChunkCoordinate &coordinate) const
		{
			const std::scoped_lock lock(_mutex);
			if (!_chunks.contains(coordinate))
			{
				throw spk::Exception("Chunk generation is not active");
			}
			return _columns.at({coordinate.x, coordinate.z});
		}

		void completeChunk(const World::ChunkCoordinate &coordinate)
		{
			if (chunk(coordinate)._columnLock.owns_lock())
			{
				const std::scoped_lock lock(_mutex);
				_columns.at({coordinate.x, coordinate.z})._ready = true;
			}
		}

		void cleanChunk(const World::ChunkCoordinate &coordinate) noexcept
		{
			const std::scoped_lock lock(_mutex);
			auto it = _chunks.find(coordinate);
			if (it == _chunks.end())
			{
				return;
			}
			if (it->second._columnLock.owns_lock())
			{
				auto &columnData = _columns.at({coordinate.x, coordinate.z});
				if (!columnData._ready)
				{
					columnData.influences = {};
					columnData.heights = {};
					columnData.strata = {};
				}
			}
			_chunks.erase(it);
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
