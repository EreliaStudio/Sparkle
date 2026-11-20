#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <utility>
#include <vector>

#include "core/context/update_context.hpp"
#include "engine/registry.hpp"
#include "engine/system.hpp"
#include "exception.hpp"
#include "world/chunk_collection.hpp"
#include "world/chunk_requester.hpp"
#include "world/world_observer.hpp"

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
		using Answer = typename ChunkRequester<TChunk>::Answer;
		using Status = typename Task<std::optional<TChunk>>::Status;

		class InterestPlanner
		{
		private:
			struct ObserverState
			{
				ChunkCoordinate center;
				ChunkInterest interest;

				[[nodiscard]] bool operator==(const ObserverState &) const = default;
			};

			using Observers = std::map<const WorldObserver *, ObserverState>;
			using Priorities = std::map<ChunkCoordinate, std::uint64_t>;

			Observers _observers;
			Priorities _priorities;
			std::vector<ChunkCoordinate> _queue;
			bool _hasPlan = false;
			std::uint64_t _revision = 0;

			[[nodiscard]] static std::uint64_t _axisDistance(std::int32_t value, std::int32_t origin, std::int32_t dimension)
			{
				const std::int64_t delta = static_cast<std::int64_t>(value) - origin;
				const std::uint64_t magnitude = static_cast<std::uint64_t>(delta < 0 ? -delta : delta);
				return magnitude * static_cast<std::uint64_t>(dimension);
			}

			static void _addDistance(std::uint64_t &distance, std::uint64_t contribution)
			{
				if (contribution > std::numeric_limits<std::uint64_t>::max() - distance)
				{
					distance = std::numeric_limits<std::uint64_t>::max();
				}
				else
				{
					distance += contribution;
				}
			}

			[[nodiscard]] static std::uint64_t _distance(const ChunkCoordinate &coordinate, const ChunkCoordinate &center)
			{
				std::uint64_t distance = 0;
				_addDistance(distance, _axisDistance(coordinate.x, center.x, TChunk::Size.x));
				_addDistance(distance, _axisDistance(coordinate.y, center.y, TChunk::Size.y));
				_addDistance(distance, _axisDistance(coordinate.z, center.z, TChunk::Size.z));
				return distance;
			}

			[[nodiscard]] static Observers _captureObservers(EngineHeadless *engine)
			{
				Observers snapshots;
				const auto &registered = Registry<WorldObserver, EngineHeadless *>::instance().elements(engine);
				for (WorldObserver *observer : registered)
				{
					if (observer->isEffectivelyActive())
					{
						snapshots.emplace(observer, ObserverState{TChunk::worldToChunkCoordinate(observer->worldPosition()), observer->interest()});
					}
				}
				return snapshots;
			}

			void _include(const ChunkCoordinate &coordinate, std::uint64_t priority)
			{
				auto [it, inserted] = _priorities.try_emplace(coordinate, priority);
				if (!inserted)
				{
					it->second = std::min(it->second, priority);
				}
			}

			void _buildPriorities()
			{
				_priorities.clear();
				for (const auto &[observer, snapshot] : _observers)
				{
					(void)observer;
					for (const ChunkCoordinate &coordinate : snapshot.interest.coordinates(snapshot.center))
					{
						_include(coordinate, _distance(coordinate, snapshot.center));
					}
				}
			}

			void _sortQueue()
			{
				_queue.clear();
				_queue.reserve(_priorities.size());
				for (const auto &[coordinate, priority] : _priorities)
				{
					(void)priority;
					_queue.push_back(coordinate);
				}
				std::stable_sort(_queue.begin(), _queue.end(), [this](const ChunkCoordinate &left, const ChunkCoordinate &right) {
					return _priorities.at(left) < _priorities.at(right);
				});
			}

			void _rebuild()
			{
				_buildPriorities();
				_sortQueue();
				++_revision;
			}

		public:
			[[nodiscard]] bool refresh(EngineHeadless *engine)
			{
				Observers snapshots = _captureObservers(engine);
				if (_hasPlan && snapshots == _observers)
				{
					return false;
				}

				_observers = std::move(snapshots);
				_rebuild();
				_hasPlan = true;
				return true;
			}

			void reset()
			{
				_observers.clear();
				_priorities.clear();
				_queue.clear();
				_hasPlan = false;
			}

			[[nodiscard]] bool contains(const ChunkCoordinate &coordinate) const
			{
				return _priorities.contains(coordinate);
			}

			[[nodiscard]] const std::vector<ChunkCoordinate> &queue() const noexcept
			{
				return _queue;
			}

			[[nodiscard]] std::uint64_t revision() const noexcept
			{
				return _revision;
			}
		};

		class RequestScheduler
		{
		private:
			struct InterestState
			{
				std::uint64_t requestId = 0;
				Duration retryAfter{};
				bool absent = false;
			};

			struct InFlight
			{
				std::uint64_t id;
				ChunkCoordinate coordinate;
				Duration submittedAt;
				Answer answer;
				bool obsolete = false;
			};

			using Interests = std::map<ChunkCoordinate, InterestState>;
			Interests _interest;
			std::vector<InFlight> _inFlight;
			std::uint64_t _lastRequestId = 0;

			[[nodiscard]] Interests _retainInterest(const InterestPlanner &planner) const
			{
				Interests result;
				for (const ChunkCoordinate &coordinate : planner.queue())
				{
					if (const auto it = _interest.find(coordinate); it != _interest.end())
					{
						result.emplace(coordinate, it->second);
					}
					else
					{
						result.try_emplace(coordinate);
					}
				}
				return result;
			}

			void _invalidateOutside(const Interests &interest)
			{
				for (InFlight &request : _inFlight)
				{
					if (!interest.contains(request.coordinate))
					{
						request.obsolete = true;
					}
				}
			}

			static void _defer(InterestState &state, Duration now, const Settings &settings)
			{
				state.requestId = 0;
				state.retryAfter = now + settings.retryDelay;
			}

			void _expirePending(InFlight &request, Duration now, const Settings &settings)
			{
				if (request.obsolete || now < request.submittedAt || now - request.submittedAt < settings.timeout)
				{
					return;
				}

				request.obsolete = true;
				if (auto it = _interest.find(request.coordinate);
					it != _interest.end() && it->second.requestId == request.id)
				{
					_defer(it->second, now, settings);
				}
			}

			static void _publishResult(InFlight &request, InterestState &state, ChunkCollection<TChunk> &collection)
			{
				const auto &result = request.answer.result();
				if (result.has_value())
				{
					collection.addChunk(request.coordinate, *result);
				}
				else
				{
					state.absent = true;
				}
			}

			void _finish(InFlight &request, Duration now, ChunkCollection<TChunk> &collection, const Settings &settings)
			{
				if (request.obsolete)
				{
					return;
				}
				auto it = _interest.find(request.coordinate);
				if (it == _interest.end() || it->second.requestId != request.id)
				{
					return;
				}

				it->second.requestId = 0;
				if (request.answer.status() == Status::Completed)
				{
					_publishResult(request, it->second, collection);
				}
				else
				{
					_defer(it->second, now, settings);
				}
			}

			[[nodiscard]] bool _canRequest(const ChunkCoordinate &coordinate, Duration now, const ChunkCollection<TChunk> &collection) const
			{
				auto it = _interest.find(coordinate);
				return it != _interest.end() &&
					   !it->second.absent &&
					   it->second.requestId == 0 &&
					   now >= it->second.retryAfter &&
					   !collection.contains(coordinate);
			}

			void _startRequest(const ChunkCoordinate &coordinate, Duration now, ChunkRequester<TChunk> &requester, const Settings &settings)
			{
				if (_lastRequestId == std::numeric_limits<std::uint64_t>::max())
				{
					throw spk::Exception("World streaming request identifier exhausted");
				}

				const std::uint64_t id = ++_lastRequestId;
				try
				{
					Answer answer = requester.request(coordinate);
					auto it = _interest.find(coordinate);
					const bool obsolete = it == _interest.end();
					if (!obsolete)
					{
						it->second.requestId = id;
					}
					_inFlight.push_back(InFlight{id, coordinate, now, std::move(answer), obsolete});
				} catch (...)
				{
					if (auto it = _interest.find(coordinate); it != _interest.end())
					{
						_defer(it->second, now, settings);
					}
				}
			}

		public:
			void reserve(std::size_t count)
			{
				_inFlight.reserve(count);
			}

			void reset()
			{
				for (InFlight &request : _inFlight)
				{
					request.obsolete = true;
				}
				_interest.clear();
			}

			void setInterest(const InterestPlanner &planner)
			{
				Interests interest = _retainInterest(planner);
				_invalidateOutside(interest);
				_interest = std::move(interest);
			}

			void collect(Duration now, ChunkCollection<TChunk> &collection, const Settings &settings)
			{
				for (std::size_t index = 0; index < _inFlight.size();)
				{
					InFlight &request = _inFlight[index];
					if (request.answer.status() == Status::Pending)
					{
						_expirePending(request, now, settings);
						++index;
						continue;
					}

					_finish(request, now, collection, settings);
					_inFlight.erase(_inFlight.begin() + static_cast<std::ptrdiff_t>(index));
				}
			}

			template <typename TCanSubmit>
			void submit(
				Duration now,
				const InterestPlanner &planner,
				ChunkCollection<TChunk> &collection,
				ChunkRequester<TChunk> &requester,
				const Settings &settings,
				TCanSubmit &&canSubmit)
			{
				const std::vector<ChunkCoordinate> pending = planner.queue();
				for (const ChunkCoordinate &coordinate : pending)
				{
					if (!canSubmit() || _inFlight.size() >= settings.maxInFlight)
					{
						return;
					}
					if (!_canRequest(coordinate, now, collection))
					{
						continue;
					}

					_startRequest(coordinate, now, requester, settings);
					collect(now, collection, settings);
				}
			}

			[[nodiscard]] std::size_t inFlightCount() const noexcept
			{
				return _inFlight.size();
			}
		};

		ChunkCollection<TChunk> &_collection;
		ChunkRequester<TChunk> &_requester;
		Settings _settings;
		ContextualizableTrait<EngineHeadless *>::OnContextEditionContract _collectionContextContract;
		InterestPlanner _planner;
		RequestScheduler _scheduler;

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
				_scheduler.setInterest(_planner);
				_unloadUnwantedChunks();
			}

			_scheduler.collect(context.time, _collection, _settings);
			_scheduler.submit(context.time, _planner, _collection, _requester, _settings, [this]() {
				return engine() != nullptr;
			});
		}

	public:
		WorldStreamingSystem(ChunkCollection<TChunk> &collection, ChunkRequester<TChunk> &requester, Settings settings = {}) :
			System("World streaming"),
			_collection(collection),
			_requester(requester),
			_settings(settings)
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
