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

			std::map<const WorldObserver *, ObserverState> _observers;
			std::map<ChunkCoordinate, std::uint64_t> _priorities;
			std::vector<ChunkCoordinate> _queue;
			bool _hasPlan = false;
			std::uint64_t _revision = 0;

			[[nodiscard]] static std::uint64_t _distance(const ChunkCoordinate &coordinate, const ChunkCoordinate &center)
			{
				std::uint64_t distance = 0;
				const auto appendAxis = [&distance](std::int32_t value, std::int32_t origin, std::int32_t dimension) {
					const std::int64_t delta = static_cast<std::int64_t>(value) - origin;
					const std::uint64_t magnitude = static_cast<std::uint64_t>(delta < 0 ? -delta : delta);
					const std::uint64_t contribution = magnitude * static_cast<std::uint64_t>(dimension);
					if (contribution > std::numeric_limits<std::uint64_t>::max() - distance)
					{
						distance = std::numeric_limits<std::uint64_t>::max();
					}
					else
					{
						distance += contribution;
					}
				};

				appendAxis(coordinate.x, center.x, TChunk::Size.x);
				appendAxis(coordinate.y, center.y, TChunk::Size.y);
				appendAxis(coordinate.z, center.z, TChunk::Size.z);
				return distance;
			}

			void _rebuild()
			{
				std::map<ChunkCoordinate, std::uint64_t> priorities;
				for (const auto &[observer, snapshot] : _observers)
				{
					(void)observer;
					for (const ChunkCoordinate &coordinate : snapshot.interest.coordinates(snapshot.center))
					{
						const std::uint64_t distance = _distance(coordinate, snapshot.center);
						auto [it, inserted] = priorities.try_emplace(coordinate, distance);
						if (!inserted && distance < it->second)
						{
							it->second = distance;
						}
					}
				}

				_priorities = std::move(priorities);
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
				++_revision;
			}

		public:
			[[nodiscard]] bool refresh(EngineHeadless *engine)
			{
				std::map<const WorldObserver *, ObserverState> snapshots;
				const auto &registered = Registry<WorldObserver, EngineHeadless *>::instance().elements(engine);
				for (WorldObserver *observer : registered)
				{
					if (!observer->isEffectivelyActive())
					{
						continue;
					}

					snapshots.emplace(observer, ObserverState{TChunk::worldToChunkCoordinate(observer->worldPosition()), observer->interest()});
				}

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

			std::map<ChunkCoordinate, InterestState> _interest;
			std::vector<InFlight> _inFlight;
			std::uint64_t _lastRequestId = 0;

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
				std::map<ChunkCoordinate, InterestState> interest;
				for (const ChunkCoordinate &coordinate : planner.queue())
				{
					if (const auto old = _interest.find(coordinate); old != _interest.end())
					{
						interest.emplace(coordinate, old->second);
					}
					else
					{
						interest.try_emplace(coordinate);
					}
				}

				for (InFlight &request : _inFlight)
				{
					if (!interest.contains(request.coordinate))
					{
						request.obsolete = true;
					}
				}
				_interest = std::move(interest);
			}

			void collect(Duration now, ChunkCollection<TChunk> &collection, const Settings &settings)
			{
				for (std::size_t index = 0; index < _inFlight.size();)
				{
					InFlight &request = _inFlight[index];
					const Status status = request.answer.status();

					if (status == Status::Pending)
					{
						if (!request.obsolete && now >= request.submittedAt && now - request.submittedAt >= settings.timeout)
						{
							request.obsolete = true;
							if (auto it = _interest.find(request.coordinate);
								it != _interest.end() && it->second.requestId == request.id)
							{
								it->second.requestId = 0;
								it->second.retryAfter = now + settings.retryDelay;
							}
						}
						++index;
						continue;
					}

					if (!request.obsolete)
					{
						if (auto it = _interest.find(request.coordinate);
							it != _interest.end() && it->second.requestId == request.id)
						{
							it->second.requestId = 0;
							if (status == Status::Completed)
							{
								const auto &result = request.answer.result();
								if (result.has_value())
								{
									collection.addChunk(request.coordinate, *result);
								}
								else
								{
									it->second.absent = true;
								}
							}
							else
							{
								it->second.retryAfter = now + settings.retryDelay;
							}
						}
					}
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
					if (!canSubmit())
					{
						return;
					}
					if (_inFlight.size() >= settings.maxInFlight)
					{
						break;
					}

					auto it = _interest.find(coordinate);
					if (it == _interest.end() ||
						it->second.absent ||
						it->second.requestId != 0 ||
						now < it->second.retryAfter ||
						collection.contains(coordinate))
					{
						continue;
					}

					if (_lastRequestId == std::numeric_limits<std::uint64_t>::max())
					{
						throw spk::Exception("World streaming request identifier exhausted");
					}

					const std::uint64_t id = ++_lastRequestId;
					try
					{
						Answer answer = requester.request(coordinate);
						it->second.requestId = id;
						_inFlight.push_back(InFlight{id, coordinate, now, std::move(answer)});
					} catch (...)
					{
						it->second.retryAfter = now + settings.retryDelay;
					}
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
