#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <vector>

#include "core/context/update_context.hpp"
#include "engine/system.hpp"
#include "exception.hpp"
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
		using Answer = typename ChunkRequester<TChunk>::Answer;
		using Status = typename Task<std::optional<TChunk>>::Status;

		struct PendingRequest
		{
			std::uint64_t generation;
			Duration deadline;
			std::optional<Answer> answer;
		};

		ChunkCollection<TChunk> &_collection;
		ChunkRequester<TChunk> &_requester;
		Settings _settings;
		ContextualizableTrait<EngineHeadless *>::OnContextEditionContract _collectionContextContract;
		detail::WorldInterestPlanner _planner;

		// Only the current request for a coordinate can publish its result.
		std::map<ChunkCoordinate, PendingRequest> _pending;
		// Old workers cannot be cancelled and still consume the physical budget.
		std::vector<Answer> _retired;
		std::map<ChunkCoordinate, Duration> _retryAt;
		std::set<ChunkCoordinate> _absent;
		std::size_t _submitting = 0;
		std::uint64_t _lastGeneration = 0;

		void _retire(PendingRequest &request)
		{
			if (request.answer.has_value() && request.answer->status() == Status::Pending)
			{
				_retired.push_back(std::move(*request.answer));
			}
		}

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

			for (auto it = _pending.begin(); it != _pending.end();)
			{
				if (!_planner.contains(it->first))
				{
					_retire(it->second);
					it = _pending.erase(it);
				}
				else
				{
					++it;
				}
			}
			std::erase_if(_retryAt, [this](const auto &entry) {
				return !_planner.contains(entry.first);
			});
			std::erase_if(_absent, [this](const ChunkCoordinate &coordinate) {
				return !_planner.contains(coordinate);
			});
			_unloadUnwantedChunks();
		}

		void _collectRetired()
		{
			std::erase_if(_retired, [](const Answer &answer) {
				return answer.status() != Status::Pending;
			});
		}

		void _collectPending(Duration now)
		{
			// Publishing can invoke user callbacks and invalidate other requests.
			// Snapshot generations so such callbacks cannot invalidate iterators.
			std::vector<std::pair<ChunkCoordinate, std::uint64_t>> snapshot;
			snapshot.reserve(_pending.size());
			for (const auto &[coordinate, request] : _pending)
			{
				if (request.answer.has_value())
				{
					snapshot.emplace_back(coordinate, request.generation);
				}
			}

			for (const auto &[coordinate, generation] : snapshot)
			{
				auto it = _pending.find(coordinate);
				if (it == _pending.end() || it->second.generation != generation)
				{
					continue;
				}

				const Status status = it->second.answer->status();
				if (status == Status::Pending && now < it->second.deadline)
				{
					continue;
				}

				if (status == Status::Completed)
				{
					Answer answer = std::move(*it->second.answer);
					_pending.erase(it);
					if (engine() != nullptr && _planner.contains(coordinate))
					{
						const auto &result = answer.result();
						if (result.has_value())
						{
							_collection.addChunk(coordinate, *result);
						}
						else
						{
							_absent.insert(coordinate);
						}
					}
				}
				else
				{
					// A timed-out task remains alive, but its result is obsolete.
					if (status == Status::Pending)
					{
						_retire(it->second);
					}
					_pending.erase(it);
					if (engine() != nullptr && _planner.contains(coordinate))
					{
						_retryAt.insert_or_assign(coordinate, now + _settings.retryDelay);
					}
				}
			}
		}

		void _submit(const ChunkCoordinate &coordinate, Duration now)
		{
			if (_pending.contains(coordinate) || inFlightCount() >= _settings.maxInFlight)
			{
				return;
			}
			if (const auto it = _retryAt.find(coordinate); it != _retryAt.end() && now < it->second)
			{
				return;
			}
			if (_lastGeneration == std::numeric_limits<std::uint64_t>::max())
			{
				throw spk::Exception("World streaming generation identifier exhausted");
			}

			const std::uint64_t generation = ++_lastGeneration;
			_pending.emplace(coordinate, PendingRequest{generation, now + _settings.timeout, std::nullopt});
			_retryAt.erase(coordinate);
			++_submitting;
			bool submitting = true;
			try
			{
				Answer answer = _requester.request(coordinate);
				--_submitting;
				submitting = false;

				auto it = _pending.find(coordinate);
				if (it != _pending.end() && it->second.generation == generation)
				{
					it->second.answer.emplace(std::move(answer));
				}
				else if (answer.status() == Status::Pending)
				{
					_retired.push_back(std::move(answer));
				}
			} catch (...)
			{
				if (submitting)
				{
					--_submitting;
				}
				auto it = _pending.find(coordinate);
				if (it != _pending.end() && it->second.generation == generation)
				{
					_pending.erase(it);
					_retryAt.insert_or_assign(coordinate, now + _settings.retryDelay);
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
			_collectRetired();
			_collectPending(context.time);

			// Index-based iteration remains valid if a publication detaches the system.
			for (std::size_t index = 0; engine() != nullptr && index < _planner.coordinates().size(); ++index)
			{
				if (inFlightCount() >= _settings.maxInFlight)
				{
					break;
				}
				const ChunkCoordinate coordinate = _planner.coordinates()[index];
				if (_absent.contains(coordinate) || _collection.contains(coordinate))
				{
					continue;
				}

				_submit(coordinate, context.time);
				_collectPending(context.time);
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
			})
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
				for (auto &[coordinate, request] : _pending)
				{
					_retire(request);
				}
				_pending.clear();
				_retryAt.clear();
				_absent.clear();
				_planner.reset();
			}
			System::attach(next);
		}

		[[nodiscard]] std::size_t inFlightCount() const noexcept
		{
			std::size_t count = _retired.size() + _submitting;
			for (const auto &entry : _pending)
			{
				if (entry.second.answer.has_value())
				{
					++count;
				}
			}
			return count;
		}

		[[nodiscard]] std::uint64_t planRevision() const noexcept
		{
			return _planner.revision();
		}
	};
}
