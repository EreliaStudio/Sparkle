#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "world/chunk_collection.hpp"
#include "world/chunk_requester.hpp"
#include "world/detail/world_interest_planner.hpp"

namespace spk::detail
{
	template <typename TChunk>
	class ChunkRequestScheduler
	{
	private:
		using Duration = std::chrono::steady_clock::duration;
		using Answer = typename ChunkRequester<TChunk>::Answer;
		using Status = typename Task<std::optional<TChunk>>::Status;

		struct InFlight
		{
			std::uint64_t id;
			ChunkCoordinate coordinate;
			Duration submittedAt;
			Answer answer;
			bool obsolete = false;
		};

		std::vector<InFlight> _inFlight;
		std::set<ChunkCoordinate> _absent;
		std::map<ChunkCoordinate, Duration> _retryAfter;
		std::uint64_t _lastRequestId = 0;

		[[nodiscard]] bool _activeFor(const ChunkCoordinate &coordinate) const
		{
			return std::any_of(_inFlight.begin(), _inFlight.end(), [&coordinate](const InFlight &request) {
				return !request.obsolete && request.coordinate == coordinate;
			});
		}

		[[nodiscard]] bool _eligible(
			const ChunkCoordinate &coordinate,
			Duration now,
			const ChunkCollection<TChunk> &collection) const
		{
			if (_absent.contains(coordinate) || collection.contains(coordinate) || _activeFor(coordinate))
			{
				return false;
			}

			const auto retry = _retryAfter.find(coordinate);
			return retry == _retryAfter.end() || now >= retry->second;
		}

		template <typename TSettings>
		void _defer(const ChunkCoordinate &coordinate, Duration now, const TSettings &settings)
		{
			_retryAfter[coordinate] = now + settings.retryDelay;
		}

		template <typename TSettings>
		void _expire(InFlight &request, Duration now, const TSettings &settings)
		{
			if (request.obsolete || now < request.submittedAt || now - request.submittedAt < settings.timeout)
			{
				return;
			}

			request.obsolete = true;
			_defer(request.coordinate, now, settings);
		}

		template <typename TSettings, typename TPublish>
		void _settle(InFlight &request, Duration now, const WorldInterestPlanner &plan, const TSettings &settings, TPublish &&publish)
		{
			if (request.obsolete || !plan.contains(request.coordinate))
			{
				return;
			}

			if (request.answer.status() == Status::Failed)
			{
				_defer(request.coordinate, now, settings);
				return;
			}

			const auto &result = request.answer.result();
			if (result.has_value())
			{
				publish(request.coordinate, *result);
			}
			else
			{
				_absent.insert(request.coordinate);
			}
		}

		template <typename TSettings>
		void _start(
			const ChunkCoordinate &coordinate,
			Duration now,
			const WorldInterestPlanner &plan,
			ChunkRequester<TChunk> &requester,
			const TSettings &settings)
		{
			if (_lastRequestId == std::numeric_limits<std::uint64_t>::max())
			{
				throw spk::Exception("World streaming request identifier exhausted");
			}

			const std::uint64_t id = ++_lastRequestId;
			try
			{
				Answer answer = requester.request(coordinate);
				_inFlight.push_back(InFlight{id, coordinate, now, std::move(answer), !plan.contains(coordinate)});
				_retryAfter.erase(coordinate);
			} catch (...)
			{
				if (plan.contains(coordinate))
				{
					_defer(coordinate, now, settings);
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
			_absent.clear();
			_retryAfter.clear();
		}

		void updateInterest(const WorldInterestPlanner &plan)
		{
			std::erase_if(_absent, [&plan](const ChunkCoordinate &coordinate) {
				return !plan.contains(coordinate);
			});
			std::erase_if(_retryAfter, [&plan](const auto &entry) {
				return !plan.contains(entry.first);
			});
			for (InFlight &request : _inFlight)
			{
				if (!plan.contains(request.coordinate))
				{
					request.obsolete = true;
				}
			}
		}

		template <typename TSettings, typename TPublish>
		void collect(Duration now, const WorldInterestPlanner &plan, const TSettings &settings, TPublish &&publish)
		{
			for (std::size_t index = 0; index < _inFlight.size();)
			{
				InFlight &request = _inFlight[index];
				if (request.answer.status() == Status::Pending)
				{
					_expire(request, now, settings);
					++index;
					continue;
				}

				_settle(request, now, plan, settings, publish);
				_inFlight.erase(_inFlight.begin() + static_cast<std::ptrdiff_t>(index));
			}
		}

		template <typename TSettings, typename TPublish, typename TCanSubmit>
		void submit(
			Duration now,
			const WorldInterestPlanner &plan,
			ChunkCollection<TChunk> &collection,
			ChunkRequester<TChunk> &requester,
			const TSettings &settings,
			TPublish &&publish,
			TCanSubmit &&canSubmit)
		{
			for (std::size_t index = 0; index < plan.coordinates().size(); ++index)
			{
				if (!canSubmit() || _inFlight.size() >= settings.maxInFlight)
				{
					return;
				}

				const ChunkCoordinate coordinate = plan.coordinates()[index];
				if (!_eligible(coordinate, now, collection))
				{
					continue;
				}

				_start(coordinate, now, plan, requester, settings);
				collect(now, plan, settings, publish);
			}
		}

		[[nodiscard]] std::size_t inFlightCount() const noexcept
		{
			return _inFlight.size();
		}
	};
}
