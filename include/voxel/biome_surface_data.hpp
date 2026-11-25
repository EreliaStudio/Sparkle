#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <utility>
#include <vector>

#include "voxel/biome.hpp"
#include "voxel/volume.hpp"
#include "world/chunk_generation_data_collection.hpp"
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
				}, [](Grid &grid) {
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
		using ColumnHandle = std::shared_ptr<const ColumnData>;

		struct ChunkData
		{
			Volume::Builder builder;
			ColumnHandle column;
		};

	private:
		struct ColumnEntry
		{
			std::mutex mutex;
			ColumnHandle result;
		};

		mutable std::mutex _columnsMutex;
		std::map<spk::Vector2Int, std::shared_ptr<ColumnEntry>> _columns;
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

		// Build and publish a complete immutable column, or leave it retryable.
		// The callback must not recursively request columns from this cache.
		template <typename TBuild>
		[[nodiscard]] ColumnHandle obtainColumn(const spk::Vector2Int &coordinate, TBuild &&build)
		{
			std::shared_ptr<ColumnEntry> entry;
			{
				const std::scoped_lock lock(_columnsMutex);
				auto it = _columns.find(coordinate);
				if (it == _columns.end())
				{
					it = _columns.emplace(coordinate, std::make_shared<ColumnEntry>()).first;
				}
				entry = it->second;
			}
			const std::scoped_lock lock(entry->mutex);
			if (!entry->result)
			{
				auto column = std::make_shared<ColumnData>();
				build(*column);
				entry->result = std::move(column);
			}
			return entry->result;
		}

		[[nodiscard]] std::size_t columnCount() const
		{
			const std::scoped_lock lock(_columnsMutex);
			return _columns.size();
		}

		// Reclaim idle columns only; active builds and externally held handles
		// keep their entries alive. Queued jobs do not need reservations.
		void flush()
		{
			std::vector<std::shared_ptr<ColumnEntry>> retired;
			{
				const std::scoped_lock lock(_columnsMutex);
				for (auto it = _columns.begin(); it != _columns.end();)
				{
					const auto &entry = it->second;
					if (entry.use_count() == 1 && (!entry->result || entry->result.use_count() == 1))
					{
						retired.push_back(std::move(it->second));
						it = _columns.erase(it);
					}
					else
					{
						++it;
					}
				}
			}
		}
	};
}
