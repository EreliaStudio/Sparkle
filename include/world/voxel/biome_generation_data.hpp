#pragma once

#include <algorithm>
#include <map>
#include <mutex>
#include <set>
#include <utility>

#include "voxel/volume.hpp"
#include "world/chunk_coordinate.hpp"
#include "world/height_map.hpp"
#include "world/voxel/strata.hpp"

namespace spk::World::Voxel
{
	template <std::size_t TCapacity>
	class Biome::GenerationData
	{
	public:
		struct World
		{
			const spk::Voxel::Definition::Registry *registry = nullptr;
		};

		struct Column
		{
			using InfluenceGrid = spk::World::SurfaceBiome::Influence::Grid<TCapacity>;
			typename InfluenceGrid::Lease influences;
			spk::World::HeightMap::Grid::Lease heights;
			Strata::Grid::Lease strata;

		private:
			friend class GenerationData;
			mutable std::mutex _mutex;
			bool _ready = false;

		public:
			// Read while the calling chunk is active, after prepareChunk().
			[[nodiscard]] bool ready() const noexcept
			{
				return _ready;
			}
		};

		struct Chunk
		{
			spk::Voxel::Volume::Builder builder;

		private:
			friend class GenerationData;
			std::unique_lock<std::mutex> _columnLock;

			explicit Chunk(spk::Voxel::Volume::Builder value) :
				builder(std::move(value))
			{
			}

		public:
			Chunk(Chunk &&) = default;
			Chunk &operator=(Chunk &&) = default;
		};

	private:
		using ColumnCollection = std::map<spk::Vector2Int, Column>;
		using ChunkCollection = std::map<spk::World::ChunkCoordinate, Chunk>;

		mutable std::mutex _mutex;
		World _world;
		ColumnCollection _columns;
		ChunkCollection _chunks;

	public:
		GenerationData() = default;

		// Bind only while the generator is idle. The registry must outlive its binding.
		void bind(const spk::Voxel::Definition::Registry *registry)
		{
			const std::scoped_lock lock(_mutex);
			if (!_chunks.empty())
			{
				throw spk::Exception("Cannot bind a voxel registry during generation");
			}
			_columns.clear();
			_world.registry = registry;
		}

		[[nodiscard]] const World &world() const noexcept
		{
			return _world;
		}

		void prepareChunk(const spk::World::ChunkCoordinate &coordinate, const spk::Vector3UInt &dimensions)
		{
			spk::Voxel::Volume::Builder builder(dimensions);
			Column *columnData;
			{
				const std::scoped_lock lock(_mutex);
				if (_chunks.contains(coordinate))
				{
					throw spk::Exception("Chunk generation is already active for this coordinate");
				}
				if (_world.registry == nullptr)
				{
					throw spk::Exception("Voxel biome generator has no bound registry");
				}
				builder.bind(*_world.registry);
				columnData = &_columns.try_emplace(spk::Vector2Int{coordinate.x, coordinate.z}).first->second;
				_chunks.try_emplace(coordinate, Chunk(std::move(builder)));
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

		[[nodiscard]] Chunk &chunk(const spk::World::ChunkCoordinate &coordinate)
		{
			const std::scoped_lock lock(_mutex);
			return _chunks.at(coordinate);
		}

		[[nodiscard]] const Chunk &chunk(const spk::World::ChunkCoordinate &coordinate) const
		{
			const std::scoped_lock lock(_mutex);
			return _chunks.at(coordinate);
		}

		// The calling surface pass must own the column lock.
		[[nodiscard]] Column &column(const spk::Vector2Int &coordinate)
		{
			const std::scoped_lock lock(_mutex);
			auto &columnData = _columns.at(coordinate);
			if (columnData._ready)
			{
				throw spk::Exception("Published column cannot be modified");
			}
			return columnData;
		}

		[[nodiscard]] const Column &column(const spk::Vector2Int &coordinate) const
		{
			const std::scoped_lock lock(_mutex);
			return _columns.at(coordinate);
		}

		// References remain valid until the caller cleans its active chunk.
		[[nodiscard]] Column &column(const spk::World::ChunkCoordinate &coordinate)
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

		[[nodiscard]] const Column &column(const spk::World::ChunkCoordinate &coordinate) const
		{
			const std::scoped_lock lock(_mutex);
			if (!_chunks.contains(coordinate))
			{
				throw spk::Exception("Chunk generation is not active");
			}
			return _columns.at({coordinate.x, coordinate.z});
		}

		// Query only for an active chunk, after prepareChunk() has synchronized the column.
		[[nodiscard]] bool surfaceReady(const spk::World::ChunkCoordinate &coordinate) const
		{
			return column(coordinate).ready();
		}

		void completeChunk(const spk::World::ChunkCoordinate &coordinate)
		{
			if (chunk(coordinate)._columnLock.owns_lock())
			{
				const std::scoped_lock lock(_mutex);
				_columns.at({coordinate.x, coordinate.z})._ready = true;
			}
		}

		void cleanChunk(const spk::World::ChunkCoordinate &coordinate) noexcept
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
