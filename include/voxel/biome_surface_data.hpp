#pragma once

#include <map>
#include <mutex>
#include <set>
#include <span>
#include <utility>
#include <vector>

#include "voxel/biome.hpp"
#include "voxel/volume.hpp"
#include "world/chunk_coordinate.hpp"
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
		mutable std::mutex _mutex;
		std::map<spk::Vector2Int, ColumnData> _columns;
		std::map<World::ChunkCoordinate, ChunkData> _chunks;

	public:
		void prepare(const World::ChunkCoordinate &coordinate, const spk::Vector3UInt &dimensions, const Definition::Registry &registry)
		{
			Volume::Builder builder(dimensions);
			builder.bind(registry);
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
				clean(coordinate);
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

		[[nodiscard]] Volume::Builder &builder(const World::ChunkCoordinate &coordinate)
		{
			return chunk(coordinate).builder;
		}

		// Only the first successful chunk generation writes the surface grids.
		[[nodiscard]] bool needsSurfaceGeneration(const World::ChunkCoordinate &coordinate) const
		{
			return chunk(coordinate)._columnLock.owns_lock();
		}

		// Mutable X/Z access is reserved for the surface passes of that first job.
		[[nodiscard]] ColumnData &column(const spk::Vector2Int &coordinate)
		{
			const std::scoped_lock lock(_mutex);
			return _columns.at(coordinate);
		}

		[[nodiscard]] const ColumnData &column(const spk::Vector2Int &coordinate) const
		{
			const std::scoped_lock lock(_mutex);
			return _columns.at(coordinate);
		}

		// References remain valid until the caller cleans its active chunk.
		[[nodiscard]] const ColumnData &column(const World::ChunkCoordinate &coordinate) const
		{
			const std::scoped_lock lock(_mutex);
			if (!_chunks.contains(coordinate))
			{
				throw spk::Exception("Chunk generation is not active");
			}
			return _columns.at({coordinate.x, coordinate.z});
		}

		void publishColumn(const World::ChunkCoordinate &coordinate)
		{
			if (needsSurfaceGeneration(coordinate))
			{
				column(spk::Vector2Int{coordinate.x, coordinate.z})._ready = true;
			}
		}

		void clean(const World::ChunkCoordinate &coordinate) noexcept
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
