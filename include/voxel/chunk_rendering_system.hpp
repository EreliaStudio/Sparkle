#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <utility>

#include "design_pattern/life_cycle.hpp"
#include "engine/entity3d.hpp"
#include "exception.hpp"
#include "rendering/engine/engine.hpp"
#include "voxel/chunk_mesher.hpp"
#include "voxel/palette.hpp"
#include "voxel/renderer.hpp"
#include "voxel/rendering_system.hpp"
#include "world/chunk_collection.hpp"

namespace spk::Voxel
{
	template <std::int32_t SX, std::int32_t SY, std::int32_t SZ>
	class ChunkRenderingSystem : public RenderingSystem
	{
	public:
		using ChunkType = Chunk<SX, SY, SZ>;
		using Collection = World::ChunkCollection<ChunkType>;
		using Coordinate = World::ChunkCoordinate;

		struct Statistics
		{
			std::size_t chunks = 0;
			std::size_t voxels = 0;
			std::size_t opaqueVertices = 0;
			std::size_t translucentVertices = 0;
			std::size_t opaqueTriangles = 0;
			std::size_t translucentTriangles = 0;
			std::chrono::nanoseconds meshingTime{0};

			[[nodiscard]] double nanosecondsPerVoxel() const noexcept
			{
				return voxels == 0 ? 0.0 : static_cast<double>(meshingTime.count()) / voxels;
			}

			[[nodiscard]] double millisecondsPerChunk() const noexcept
			{
				return chunks == 0 ? 0.0 : static_cast<double>(meshingTime.count()) / (1000000.0 * chunks);
			}
		};

	private:
		struct Tracked
		{
			Entity3D *entity;
			LifeCycle::DestructionContract destruction;
			bool ownsRenderer;
		};

		Collection *_collection = nullptr;
		std::optional<spk::Palette> _palette;
		typename Collection::EditionContract _edition;
		LifeCycle::DestructionContract _collectionDestruction;
		std::map<Coordinate, Tracked> _tracked;
		std::set<Coordinate> _dirty;
		Statistics _lastStatistics;

		void _mark(const Coordinate &coordinate)
		{
			_dirty.insert(coordinate);
			constexpr auto Min = std::numeric_limits<std::int32_t>::min();
			constexpr auto Max = std::numeric_limits<std::int32_t>::max();
			if (coordinate.x > Min)
			{
				_dirty.insert({coordinate.x - 1, coordinate.y, coordinate.z});
			}
			if (coordinate.x < Max)
			{
				_dirty.insert({coordinate.x + 1, coordinate.y, coordinate.z});
			}
			if (coordinate.y > Min)
			{
				_dirty.insert({coordinate.x, coordinate.y - 1, coordinate.z});
			}
			if (coordinate.y < Max)
			{
				_dirty.insert({coordinate.x, coordinate.y + 1, coordinate.z});
			}
			if (coordinate.z > Min)
			{
				_dirty.insert({coordinate.x, coordinate.y, coordinate.z - 1});
			}
			if (coordinate.z < Max)
			{
				_dirty.insert({coordinate.x, coordinate.y, coordinate.z + 1});
			}
		}

		void _track()
		{
			std::set<Coordinate> present;
			for (auto &[coordinate, entry] : *_collection)
			{
				present.insert(coordinate);
				auto found = _tracked.find(coordinate);
				if (found != _tracked.end() && found->second.entity == &entry.entity() && found->second.destruction.isValid())
				{
					continue;
				}
				_tracked.erase(coordinate);
				const bool ownsRenderer = entry.entity().getComponent<Renderer>() == nullptr;
				auto destruction = entry.entity().subscribeToDestruction([this, coordinate]() {
					_mark(coordinate);
				});
				_tracked.emplace(coordinate, Tracked{&entry.entity(), std::move(destruction), ownsRenderer});
			}
			for (auto it = _tracked.begin(); it != _tracked.end();)
			{
				if (!present.contains(it->first))
				{
					it = _tracked.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

	public:
		ChunkRenderingSystem() = default;

		~ChunkRenderingSystem() override
		{
			unbind();
		}

		ChunkRenderingSystem(const ChunkRenderingSystem &) = delete;
		ChunkRenderingSystem &operator=(const ChunkRenderingSystem &) = delete;

		void bind(Collection &collection, spk::Palette palette)
		{
			if (_collection != nullptr)
			{
				throw spk::Exception("Voxel chunk rendering system is already bound");
			}
			if (dynamic_cast<spk::Engine *>(engine()) == nullptr || collection.engine() != engine())
			{
				throw spk::Exception("Voxel chunk rendering requires a collection attached to the same Graphics Engine");
			}
			_collection = &collection;
			_palette.emplace(std::move(palette));
			_edition = collection.subscribeToChunkEdition([this](const Coordinate &coordinate) {
				_mark(coordinate);
			});
			_collectionDestruction = collection.root().subscribeToDestruction([this]() {
				unbind();
			});
			for (const auto &[coordinate, entry] : collection)
			{
				(void)entry;
				_mark(coordinate);
			}
		}

		void unbind()
		{
			_collectionDestruction.resign();
			_edition.resign();
			for (auto &[coordinate, tracked] : _tracked)
			{
				(void)coordinate;
				if (tracked.ownsRenderer && tracked.destruction.isValid())
				{
					if (Renderer *renderer = tracked.entity->getComponent<Renderer>(); renderer != nullptr)
					{
						tracked.entity->removeComponent(*renderer);
					}
				}
			}
			_tracked.clear();
			_dirty.clear();
			_palette.reset();
			_collection = nullptr;
		}

		void attach(spk::EngineHeadless *next) override
		{
			if (next != engine())
			{
				unbind();
			}
			RenderingSystem::attach(next);
		}

		void synchronize()
		{
			if (_collection == nullptr || _dirty.empty())
			{
				return;
			}
			if (_collection->engine() != engine())
			{
				throw spk::Exception("Voxel chunk rendering collection is no longer attached to this Engine");
			}
			_track();
			Statistics next;
			const auto work = std::exchange(_dirty, {});
			for (const Coordinate &coordinate : work)
			{
				auto found = _tracked.find(coordinate);
				if (found == _tracked.end())
				{
					continue;
				}

				ChunkType &chunk = _collection->at(coordinate);
				ChunkMesher<SX, SY, SZ> mesher(*_collection, coordinate);
				const auto start = std::chrono::steady_clock::now();
				Geometry geometry = mesher.bake(chunk.volume());
				next.meshingTime += std::chrono::duration_cast<std::chrono::nanoseconds>(
					std::chrono::steady_clock::now() - start);
				++next.chunks;
				next.voxels += static_cast<std::size_t>(SX) * SY * SZ;
				next.opaqueVertices += geometry.opaqueMesh().vertexCount();
				next.translucentVertices += geometry.transparentMesh().vertexCount();
				next.opaqueTriangles += geometry.opaqueMesh().indexCount() / 3;
				next.translucentTriangles += geometry.transparentMesh().indexCount() / 3;

				Entity3D &entity = *found->second.entity;
				Renderer *renderer = entity.getComponent<Renderer>();
				if (renderer == nullptr)
				{
					renderer = &entity.addComponent<Renderer>();
				}
				renderer->bind(*_palette);
				renderer->bind(std::move(geometry));
			}
			_lastStatistics = next;
		}

		[[nodiscard]] const Statistics &lastStatistics() const noexcept
		{
			return _lastStatistics;
		}

		void buildViewRenderSnapshot(
			spk::RenderSnapshot::Builder &builder,
			const spk::EngineView::CaptureContext &context) override
		{
			synchronize();
			RenderingSystem::buildViewRenderSnapshot(builder, context);
		}
	};
}
