#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>

#include "core/context/update_context.hpp"
#include "engine/system.hpp"
#include "exception.hpp"
#include "threading/request_coordinator.hpp"
#include "world/chunk_collection.hpp"
#include "world/chunk_requester.hpp"
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
		using Coordinator = RequestCoordinator<ChunkCoordinate, std::optional<TChunk>>;

		ChunkCollection<TChunk> &_collection;
		ChunkRequester<TChunk> &_requester;
		Settings _settings;
		ContextualizableTrait<EngineHeadless *>::OnContextEditionContract _collectionContextContract;
		detail::WorldInterestPlanner _planner;
		Coordinator _requests;
		std::set<ChunkCoordinate> _absent;
		std::optional<Duration> _lastUpdateTime;

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

		void _refreshInterests()
		{
			if (!_planner.refresh(engine()))
			{
				return;
			}

			_requests.retain([this](const ChunkCoordinate &coordinate) {
				return _planner.contains(coordinate);
			});
			std::erase_if(_absent, [this](const ChunkCoordinate &coordinate) {
				return !_planner.contains(coordinate);
			});
			_unloadUnwantedChunks();
		}

		void _updateClock(Duration now)
		{
			const Duration delta = _lastUpdateTime.has_value() && now > *_lastUpdateTime ? now - *_lastUpdateTime : Duration::zero();
			_lastUpdateTime = now;
			_requests.advance(delta);
		}

		void _updateState(UpdateContext &context) override
		{
			if (engine() == nullptr)
			{
				return;
			}

			_refreshInterests();
			_updateClock(context.time);

			const auto publish = [this](const ChunkCoordinate &coordinate, const std::optional<TChunk> &result) {
				if (engine() == nullptr || !_planner.contains(coordinate))
				{
					return;
				}
				if (result.has_value())
				{
					_collection.addChunk(coordinate, *result);
				}
				else
				{
					_absent.insert(coordinate);
				}
			};

			_requests.collect(context.time, publish);
			for (const ChunkCoordinate &coordinate : _planner.coordinates())
			{
				if (engine() == nullptr || _requests.inFlightCount() >= _settings.maxInFlight)
				{
					break;
				}
				if (_absent.contains(coordinate) || _collection.contains(coordinate))
				{
					continue;
				}

				(void)_requests.request(coordinate, context.time, [this, &coordinate]() {
					return _requester.request(coordinate);
				});
				_requests.collect(context.time, publish);
			}
		}

	public:
		WorldStreamingSystem(ChunkCollection<TChunk> &collection, ChunkRequester<TChunk> &requester, Settings settings = {}) :
			System("World streaming"),
			_collection(collection),
			_requester(requester),
			_settings(settings),
			_planner(TChunk::Size, [](const Vector3 &world) {
				return TChunk::worldToChunkCoordinate(world);
			}),
			_requests(_settings.maxInFlight, _settings.timeout, [this](const ChunkCoordinate &, typename Coordinator::Failure) {
				return std::optional<Duration>(_settings.retryDelay);
			})
		{
			if (_settings.retryDelay <= Duration::zero())
			{
				throw spk::Exception("World streaming requires a positive retry delay");
			}
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
				_requests.invalidateAll();
				_absent.clear();
				_lastUpdateTime.reset();
				_planner.reset();
			}
			System::attach(next);
		}

		[[nodiscard]] std::size_t inFlightCount() const noexcept
		{
			return _requests.inFlightCount();
		}

		[[nodiscard]] std::uint64_t planRevision() const noexcept
		{
			return _planner.revision();
		}
	};
}
