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
		using Clock = std::chrono::steady_clock;
		using Duration = Clock::duration;
		using Answer = typename ChunkRequester<TChunk>::Answer;
		using Status = typename Task<std::optional<TChunk>>::Status;

		struct ObserverState
		{
			ChunkCoordinate center;
			ChunkInterest interest;

			[[nodiscard]] bool operator==(const ObserverState &) const = default;
		};

		struct InterestState
		{
			std::uint64_t priority = 0;
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

		ChunkCollection<TChunk> &_collection;
		ChunkRequester<TChunk> &_requester;
		Settings _settings;
		ContextualizableTrait<EngineHeadless *>::OnContextEditionContract _collectionContextContract;
		std::map<const WorldObserver *, ObserverState> _observers;
		std::map<ChunkCoordinate, InterestState> _interest;
		std::vector<ChunkCoordinate> _queue;
		std::vector<InFlight> _inFlight;
		bool _hasPlan = false;
		std::uint64_t _lastRequestId = 0;
		std::uint64_t _planRevision = 0;

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

		void _invalidate()
		{
			for (InFlight &request : _inFlight)
			{
				request.obsolete = true;
			}
			_observers.clear();
			_interest.clear();
			_queue.clear();
			_hasPlan = false;
		}

		void _rebuild()
		{
			std::map<ChunkCoordinate, InterestState> nextInterest;
			for (const auto &[observer, snapshot] : _observers)
			{
				(void)observer;
				for (const ChunkCoordinate &coordinate : snapshot.interest.coordinates(snapshot.center))
				{
					const std::uint64_t distance = _distance(coordinate, snapshot.center);
					auto [it, inserted] = nextInterest.try_emplace(coordinate);
					if (inserted || distance < it->second.priority)
					{
						it->second.priority = distance;
					}
				}
			}

			for (auto &[coordinate, state] : nextInterest)
			{
				if (const auto old = _interest.find(coordinate); old != _interest.end())
				{
					state.requestId = old->second.requestId;
					state.retryAfter = old->second.retryAfter;
					state.absent = old->second.absent;
				}
			}

			for (InFlight &request : _inFlight)
			{
				if (!nextInterest.contains(request.coordinate))
				{
					request.obsolete = true;
				}
			}

			_interest = std::move(nextInterest);
			for (auto it = _collection.begin(); it != _collection.end();)
			{
				const ChunkCoordinate coordinate = it->first;
				++it;
				if (!_interest.contains(coordinate))
				{
					(void)_collection.removeChunk(coordinate);
				}
			}

			_queue.clear();
			_queue.reserve(_interest.size());
			for (const auto &[coordinate, state] : _interest)
			{
				(void)state;
				_queue.push_back(coordinate);
			}
			std::stable_sort(_queue.begin(), _queue.end(), [this](const ChunkCoordinate &left, const ChunkCoordinate &right) {
				return _interest.at(left).priority < _interest.at(right).priority;
			});
			++_planRevision;
		}

		void _refreshObservers()
		{
			std::map<const WorldObserver *, ObserverState> snapshots;
			const auto &registered = Registry<WorldObserver, EngineHeadless *>::instance().elements(engine());
			for (WorldObserver *observer : registered)
			{
				if (!observer->isEffectivelyActive())
				{
					continue;
				}

				snapshots.emplace(observer, ObserverState{
											  TChunk::worldToChunkCoordinate(observer->worldPosition()),
											  observer->interest()});
			}
			if (!_hasPlan || snapshots != _observers)
			{
				_observers = std::move(snapshots);
				_rebuild();
				_hasPlan = true;
			}
		}

		void _collect(Duration now)
		{
			for (std::size_t index = 0; index < _inFlight.size();)
			{
				InFlight &request = _inFlight[index];
				const Status status = request.answer.status();

				if (status == Status::Pending)
				{
					if (!request.obsolete && now >= request.submittedAt && now - request.submittedAt >= _settings.timeout)
					{
						request.obsolete = true;
						if (auto it = _interest.find(request.coordinate);
							it != _interest.end() && it->second.requestId == request.id)
						{
							it->second.requestId = 0;
							it->second.retryAfter = now + _settings.retryDelay;
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
								_collection.addChunk(request.coordinate, *result);
							}
							else
							{
								it->second.absent = true;
							}
						}
						else
						{
							it->second.retryAfter = now + _settings.retryDelay;
						}
					}
				}
				_inFlight.erase(_inFlight.begin() + static_cast<std::ptrdiff_t>(index));
			}
		}

		void _submit(Duration now)
		{
			const std::vector<ChunkCoordinate> pending = _queue;
			for (const ChunkCoordinate &coordinate : pending)
			{
				if (engine() == nullptr)
				{
					return;
				}
				if (_inFlight.size() >= _settings.maxInFlight)
				{
					break;
				}

				auto it = _interest.find(coordinate);
				if (it == _interest.end() ||
					it->second.absent ||
					it->second.requestId != 0 ||
					now < it->second.retryAfter ||
					_collection.contains(coordinate))
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
					Answer answer = _requester.request(coordinate);
					it->second.requestId = id;
					_inFlight.push_back(InFlight{id, coordinate, now, std::move(answer)});
				} catch (...)
				{
					it->second.retryAfter = now + _settings.retryDelay;
				}
				_collect(now);
			}
		}

		void _updateState(UpdateContext &context) override
		{
			if (engine() == nullptr)
			{
				return;
			}

			_refreshObservers();
			_collect(context.time);
			_submit(context.time);
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
			_inFlight.reserve(_settings.maxInFlight);
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
				_invalidate();
			}
			System::attach(next);
		}

		[[nodiscard]] std::size_t inFlightCount() const noexcept
		{
			return _inFlight.size();
		}

		[[nodiscard]] std::uint64_t planRevision() const noexcept
		{
			return _planRevision;
		}
	};
}
