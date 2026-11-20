#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "core/context/update_context.hpp"
#include "engine/system.hpp"
#include "exception.hpp"
#include "world/chunk_collection.hpp"
#include "world/chunk_requester.hpp"
#include "world/detail/chunk_request_scheduler.hpp"
#include "world/detail/world_interest_planner.hpp"

namespace spk
{
	template <typename TChunk>
	class WorldStreamingSystem : public System
	{
	public:
		struct Settings
		{
			std::size_t maxInFlight = 4;
			std::chrono::steady_clock::duration timeout = std::chrono::seconds(10);
			std::chrono::steady_clock::duration retryDelay = std::chrono::milliseconds(500);
		};

	private:
		using Duration = std::chrono::steady_clock::duration;

		ChunkCollection<TChunk> &_collection;
		ChunkRequester<TChunk> &_requester;
		Settings _settings;
		ContextualizableTrait<EngineHeadless *>::OnContextEditionContract _collectionContextContract;
		detail::WorldInterestPlanner _planner;
		detail::ChunkRequestScheduler<TChunk> _scheduler;

		void _unloadUnwantedChunks()
		{
			for (auto it = _collection.begin(); it != _collection.end();)
			{
				const ChunkCoordinate coordinate = it->first;
				++it;
				if (!_planner.contains(coordinate))
				{
					(void)_collection.removeChunk(coordinate);
				}
			}
		}

		void _updateState(UpdateContext &context) override
		{
			if (engine() == nullptr)
			{
				return;
			}
			if (_planner.refresh(engine()))
			{
				_scheduler.updateInterest(_planner);
				_unloadUnwantedChunks();
			}

			const auto publish = [this](const ChunkCoordinate &coordinate, const TChunk &chunk) {
				_collection.addChunk(coordinate, chunk);
			};
			_scheduler.collect(context.time, _planner, _settings, publish);
			_scheduler.submit(context.time, _planner, _collection, _requester, _settings, publish, [this]() {
				return engine() != nullptr;
			});
		}

	public:
		WorldStreamingSystem(ChunkCollection<TChunk> &collection, ChunkRequester<TChunk> &requester, Settings settings = {}) :
			System("World streaming"),
			_collection(collection),
			_requester(requester),
			_settings(settings),
			_planner(TChunk::Size, [](const Vector3 &world) {
				return TChunk::worldToChunkCoordinate(world);
			})
		{
			if (_settings.maxInFlight == 0 || _settings.timeout <= Duration::zero() || _settings.retryDelay <= Duration::zero())
			{
				throw spk::Exception("World streaming requires a positive budget, timeout and retry delay");
			}
			_scheduler.reserve(_settings.maxInFlight);
			_collectionContextContract = _collection.root().subscribeToContextEdition(
				[this](EngineHeadless *const &, EngineHeadless *const &next) {
					if (engine() != nullptr && engine() != next)
					{
						attach(nullptr);
					}
				});
		}

		WorldStreamingSystem(const WorldStreamingSystem &) = delete;
		WorldStreamingSystem &operator=(const WorldStreamingSystem &) = delete;
		WorldStreamingSystem(WorldStreamingSystem &&) = delete;
		WorldStreamingSystem &operator=(WorldStreamingSystem &&) = delete;

		void attach(EngineHeadless *next) override
		{
			if (next == engine())
			{
				return;
			}
			if (next != nullptr && engine() != nullptr)
			{
				throw spk::Exception("World streaming must detach before changing Engine");
			}
			if (next != nullptr && _collection.engine() != next)
			{
				throw spk::Exception("World streaming collection must be attached to the same Engine");
			}
			if (next == nullptr)
			{
				_scheduler.reset();
				_planner.reset();
			}
			System::attach(next);
		}

		[[nodiscard]] std::size_t inFlightCount() const noexcept
		{
			return _scheduler.inFlightCount();
		}

		[[nodiscard]] std::uint64_t planRevision() const noexcept
		{
			return _planner.revision();
		}
	};
}
