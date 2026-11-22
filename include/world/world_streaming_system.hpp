#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "container/priority_queue.hpp"
#include "core/context/update_context.hpp"
#include "engine/registry.hpp"
#include "engine/system.hpp"
#include "exception.hpp"
#include "world/chunk_collection.hpp"
#include "world/chunk_interest.hpp"
#include "world/chunk_requester.hpp"
#include "world/world_observer.hpp"

namespace spk::World
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

		// Owns the desired chunk union and its priority order, not task state.
		class InterestPlan
		{
		private:
			struct ObserverState
			{
				ChunkCoordinate center;
				ChunkInterest interest;

				[[nodiscard]] bool operator==(const ObserverState &) const = default;
			};

			using ObserverStateCollection = std::map<const WorldObserver *, ObserverState>;
			using ChunkPriorityCollection = std::map<ChunkCoordinate, std::uint64_t>;

			struct Comparator
			{
				const ChunkPriorityCollection *priorities;

				[[nodiscard]] bool operator()(const ChunkCoordinate &left, const ChunkCoordinate &right) const
				{
					const auto a = priorities->at(left);
					const auto b = priorities->at(right);
					return a == b ? left < right : a < b;
				}
			};

			ObserverStateCollection _observers;
			ChunkPriorityCollection _priorities;
			PriorityQueue<ChunkCoordinate, Comparator> _coordinates;
			bool _initialized = false;
			std::uint64_t _revision = 0;

			[[nodiscard]] static ObserverStateCollection _capture(EngineHeadless *engine)
			{
				ObserverStateCollection states;
				const auto &registered = Registry<WorldObserver, EngineHeadless *>::instance().elements(engine);
				for (WorldObserver *observer : registered)
				{
					if (observer->isEffectivelyActive())
					{
						states.emplace(observer, ObserverState{TChunk::worldToChunkCoordinate(observer->worldPosition()), observer->interest()});
					}
				}
				return states;
			}

			[[nodiscard]] static std::uint64_t _axisDistance(std::int32_t value, std::int32_t center, std::int32_t size)
			{
				const std::int64_t difference = static_cast<std::int64_t>(value) - center;
				const std::uint64_t magnitude = static_cast<std::uint64_t>(difference < 0 ? -difference : difference);
				return magnitude * static_cast<std::uint64_t>(size);
			}

			[[nodiscard]] static std::uint64_t _distance(const ChunkCoordinate &coordinate, const ChunkCoordinate &center)
			{
				std::uint64_t distance = 0;
				const auto add = [&distance](std::uint64_t value) {
					distance += std::min(value, std::numeric_limits<std::uint64_t>::max() - distance);
				};
				add(_axisDistance(coordinate.x, center.x, TChunk::Size.x));
				add(_axisDistance(coordinate.y, center.y, TChunk::Size.y));
				add(_axisDistance(coordinate.z, center.z, TChunk::Size.z));
				return distance;
			}

			static void _extend(ChunkPriorityCollection &priorities, const ObserverState &state)
			{
				for (const ChunkCoordinate &coordinate : state.interest.coordinates(state.center))
				{
					const std::uint64_t distance = _distance(coordinate, state.center);
					auto [it, inserted] = priorities.try_emplace(coordinate, distance);
					if (!inserted)
					{
						it->second = std::min(it->second, distance);
					}
				}
			}

			void _rebuild(const ObserverStateCollection &observers)
			{
				ChunkPriorityCollection priorities;
				for (const auto &[observer, state] : observers)
				{
					(void)observer;
					_extend(priorities, state);
				}

				std::vector<ChunkCoordinate> coordinates;
				coordinates.reserve(priorities.size());
				for (const auto &[coordinate, distance] : priorities)
				{
					(void)distance;
					coordinates.push_back(coordinate);
				}

				_priorities = std::move(priorities);
				_coordinates.clear();
				_coordinates.pushBack(std::move(coordinates));
			}

		public:
			InterestPlan() :
				_coordinates(Comparator{&_priorities})
			{
			}

			[[nodiscard]] bool refresh(EngineHeadless *engine)
			{
				ObserverStateCollection current = _capture(engine);
				if (_initialized && current == _observers)
				{
					return false;
				}
				_rebuild(current);
				_observers = std::move(current);
				_initialized = true;
				++_revision;
				return true;
			}

			void reset()
			{
				_observers.clear();
				_priorities.clear();
				_coordinates.clear();
				_initialized = false;
			}

			[[nodiscard]] bool contains(const ChunkCoordinate &coordinate) const
			{
				return _priorities.contains(coordinate);
			}

			[[nodiscard]] const auto &coordinates() const noexcept
			{
				return _coordinates;
			}

			[[nodiscard]] std::uint64_t revision() const noexcept
			{
				return _revision;
			}
		};

		// Owns only this system's task lifetimes, retries and absence cache.
		class ChunkRequestManager
		{
		private:
			struct Pending
			{
				std::uint64_t generation;
				Duration deadline;
				std::optional<Answer> answer;
			};

			using PendingMap = std::map<ChunkCoordinate, Pending>;

			PendingMap _pending;
			std::vector<Answer> _retired;
			std::map<ChunkCoordinate, Duration> _retryAt;
			std::set<ChunkCoordinate> _absent;
			std::size_t _submitting = 0;
			std::uint64_t _lastGeneration = 0;

			void _retire(Pending &request)
			{
				if (request.answer.has_value() && request.answer->status() == Status::Pending)
				{
					_retired.push_back(std::move(*request.answer));
				}
			}

			void _prunePending(const InterestPlan &interest)
			{
				for (auto it = _pending.begin(); it != _pending.end();)
				{
					if (interest.contains(it->first))
					{
						++it;
						continue;
					}
					_retire(it->second);
					it = _pending.erase(it);
				}
			}

			void _pruneCache(const InterestPlan &interest)
			{
				std::erase_if(_retryAt, [&interest](const auto &entry) {
					return !interest.contains(entry.first);
				});
				std::erase_if(_absent, [&interest](const ChunkCoordinate &coordinate) {
					return !interest.contains(coordinate);
				});
			}

			[[nodiscard]] std::vector<std::pair<ChunkCoordinate, std::uint64_t>> _snapshot() const
			{
				std::vector<std::pair<ChunkCoordinate, std::uint64_t>> snapshot;
				snapshot.reserve(_pending.size());
				for (const auto &[coordinate, request] : _pending)
				{
					if (request.answer.has_value())
					{
						snapshot.emplace_back(coordinate, request.generation);
					}
				}
				return snapshot;
			}

			void _fail(const ChunkCoordinate &coordinate, PendingMap::iterator it, Duration retryAt)
			{
				_retire(it->second);
				_pending.erase(it);
				_retryAt.insert_or_assign(coordinate, retryAt);
			}

			template <typename TCallback>
			void _poll(const ChunkCoordinate &coordinate, std::uint64_t generation, Duration now, Duration retryDelay, TCallback &onResult)
			{
				auto it = _pending.find(coordinate);
				if (it == _pending.end() || it->second.generation != generation)
				{
					return;
				}

				const Status status = it->second.answer->status();
				if (status == Status::Pending && now < it->second.deadline)
				{
					return;
				}
				if (status != Status::Completed)
				{
					_fail(coordinate, it, now + retryDelay);
					return;
				}

				Answer answer = std::move(*it->second.answer);
				_pending.erase(it);
				onResult(coordinate, answer.result());
			}

			[[nodiscard]] std::uint64_t _begin(const ChunkCoordinate &coordinate, Duration deadline)
			{
				if (_lastGeneration == std::numeric_limits<std::uint64_t>::max())
				{
					throw spk::Exception("World streaming generation identifier exhausted");
				}
				const std::uint64_t generation = ++_lastGeneration;
				_pending.emplace(coordinate, Pending{generation, deadline, std::nullopt});
				_retryAt.erase(coordinate);
				++_submitting;
				return generation;
			}

			void _accept(const ChunkCoordinate &coordinate, std::uint64_t generation, Answer answer)
			{
				auto it = _pending.find(coordinate);
				if (it != _pending.end() && it->second.generation == generation)
				{
					it->second.answer.emplace(std::move(answer));
				}
				else if (answer.status() == Status::Pending)
				{
					_retired.push_back(std::move(answer));
				}
			}

			void _reject(const ChunkCoordinate &coordinate, std::uint64_t generation, Duration retryAt)
			{
				auto it = _pending.find(coordinate);
				if (it != _pending.end() && it->second.generation == generation)
				{
					_pending.erase(it);
					_retryAt.insert_or_assign(coordinate, retryAt);
				}
			}

		public:
			void retain(const InterestPlan &interest)
			{
				_prunePending(interest);
				_pruneCache(interest);
			}

			void reset()
			{
				for (auto &[coordinate, request] : _pending)
				{
					(void)coordinate;
					_retire(request);
				}
				_pending.clear();
				_retryAt.clear();
				_absent.clear();
			}

			void collectRetired()
			{
				std::erase_if(_retired, [](const Answer &answer) {
					return answer.status() != Status::Pending;
				});
			}

			template <typename TCallback>
			void collect(Duration now, Duration retryDelay, TCallback &&onResult)
			{
				for (const auto &[coordinate, generation] : _snapshot())
				{
					_poll(coordinate, generation, now, retryDelay, onResult);
				}
			}

			[[nodiscard]] bool canSubmit(const ChunkCoordinate &coordinate, Duration now, std::size_t budget) const
			{
				if (inFlightCount() >= budget || _pending.contains(coordinate) || _absent.contains(coordinate))
				{
					return false;
				}
				const auto it = _retryAt.find(coordinate);
				return it == _retryAt.end() || now >= it->second;
			}

			void submit(const ChunkCoordinate &coordinate, Duration now, const Settings &settings, ChunkRequester<TChunk> &requester)
			{
				const std::uint64_t generation = _begin(coordinate, now + settings.timeout);
				bool submitting = true;
				try
				{
					Answer answer = requester.request(coordinate);
					--_submitting;
					submitting = false;
					_accept(coordinate, generation, std::move(answer));
				} catch (...)
				{
					if (submitting)
					{
						--_submitting;
					}
					_reject(coordinate, generation, now + settings.retryDelay);
				}
			}

			void markAbsent(const ChunkCoordinate &coordinate)
			{
				_absent.insert(coordinate);
			}

			[[nodiscard]] std::size_t inFlightCount() const noexcept
			{
				std::size_t count = _retired.size() + _submitting;
				for (const auto &[coordinate, request] : _pending)
				{
					(void)coordinate;
					if (request.answer.has_value())
					{
						++count;
					}
				}
				return count;
			}
		};

		ChunkCollection<TChunk> &_collection;
		ChunkRequester<TChunk> &_requester;
		Settings _settings;
		ContextualizableTrait<EngineHeadless *>::OnContextEditionContract _collectionContextContract;
		InterestPlan _interest;
		ChunkRequestManager _requestManager;

		void _unloadUnwanted()
		{
			for (auto it = _collection.begin(); it != _collection.end();)
			{
				const ChunkCoordinate coordinate = it->first;
				++it;
				if (!_interest.contains(coordinate))
				{
					(void)_collection.removeChunk(coordinate);
				}
			}
		}

		void _refreshInterests()
		{
			if (_interest.refresh(engine()))
			{
				_requestManager.retain(_interest);
				_unloadUnwanted();
			}
		}

		void _publish(const ChunkCoordinate &coordinate, const std::optional<TChunk> &result)
		{
			if (engine() == nullptr || !_interest.contains(coordinate))
			{
				return;
			}
			if (result.has_value())
			{
				_collection.addChunk(coordinate, *result);
			}
			else
			{
				_requestManager.markAbsent(coordinate);
			}
		}

		void _collect(Duration now)
		{
			_requestManager.collect(now, _settings.retryDelay, [this](const ChunkCoordinate &coordinate, const std::optional<TChunk> &result) {
				_publish(coordinate, result);
			});
		}

		void _schedule(Duration now)
		{
			for (std::size_t index = 0; engine() != nullptr && index < _interest.coordinates().size(); ++index)
			{
				if (_requestManager.inFlightCount() >= _settings.maxInFlight)
				{
					break;
				}
				const ChunkCoordinate coordinate = _interest.coordinates()[index];
				if (!_collection.contains(coordinate) && _requestManager.canSubmit(coordinate, now, _settings.maxInFlight))
				{
					_requestManager.submit(coordinate, now, _settings, _requester);
					_collect(now);
				}
			}
		}

		void _updateState(UpdateContext &context) override
		{
			if (engine() == nullptr)
			{
				return;
			}
			_refreshInterests();
			_requestManager.collectRetired();
			_collect(context.time);
			_schedule(context.time);
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
				_requestManager.reset();
				_interest.reset();
			}
			System::attach(next);
		}

		[[nodiscard]] std::size_t inFlightCount() const noexcept
		{
			return _requestManager.inFlightCount();
		}

		[[nodiscard]] std::uint64_t planRevision() const noexcept
		{
			return _interest.revision();
		}
	};
}
