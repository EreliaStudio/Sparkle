#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include "core/platform/controlled_timer.hpp"
#include "exception.hpp"
#include "threading/task_supervisor.hpp"

namespace spk
{
	// Does not queue requests: callers offer keys in priority order.
	// Expired and invalidated workers retain their physical budget until settled.
	template <typename TKey, typename TResult>
	class RequestCoordinator
	{
	public:
		using Duration = Timer::Duration;
		using Supervisor = TaskSupervisor<TResult>;
		using Id = typename Supervisor::Id;
		using Answer = typename Supervisor::Answer;
		using Status = typename Supervisor::Status;

		enum class Failure
		{
			Timeout,
			TaskFailed,
			SubmissionFailed
		};

		// nullopt disables further attempts until the key is invalidated.
		using RetryPolicy = std::function<std::optional<Duration>(const TKey &, Failure)>;

	private:
		enum class Phase
		{
			Submitting,
			Active,
			Waiting,
			Disabled
		};

		struct Request
		{
			std::uint64_t generation;
			Phase phase = Phase::Submitting;
			Id id = 0;
			Duration retryAt = Duration::zero();
		};

		std::map<TKey, Request> _requests;
		Supervisor _supervisor;
		std::size_t _maxInFlight;
		std::size_t _submitting = 0;
		Duration _timeout;
		Duration _now = Duration::zero();
		RetryPolicy _retryPolicy;
		std::uint64_t _lastGeneration = 0;

		void _invalidate(const Request &request) noexcept
		{
			if (request.phase == Phase::Active)
			{
				_supervisor.invalidate(request.id);
			}
		}

		void _fail(const TKey &key, std::uint64_t generation, Failure reason, Duration now)
		{
			auto it = _requests.find(key);
			if (it == _requests.end() || it->second.generation != generation)
			{
				return;
			}

			// Disable first: the user-supplied policy may reenter the coordinator.
			it->second.phase = Phase::Disabled;
			const auto delay = _retryPolicy ? _retryPolicy(key, reason) : std::nullopt;
			if (!delay.has_value())
			{
				return;
			}
			if (*delay < Duration::zero())
			{
				throw spk::Exception("RequestCoordinator retry delay cannot be negative");
			}

			it = _requests.find(key);
			if (it != _requests.end() && it->second.generation == generation)
			{
				it->second.phase = Phase::Waiting;
				it->second.retryAt = now + *delay;
			}
		}

		void _timeoutRequest(const TKey &key, Id id)
		{
			auto it = _requests.find(key);
			if (it != _requests.end() && it->second.phase == Phase::Active && it->second.id == id)
			{
				_fail(key, it->second.generation, Failure::Timeout, _now);
			}
		}

	public:
		RequestCoordinator(std::size_t maxInFlight, Duration timeout, RetryPolicy retryPolicy = {}) :
			_maxInFlight(maxInFlight),
			_timeout(timeout),
			_retryPolicy(std::move(retryPolicy))
		{
			if (_maxInFlight == 0 || _timeout <= Duration::zero())
			{
				throw spk::Exception("RequestCoordinator requires a positive budget and timeout");
			}
		}

		void advance(Duration deltaTime)
		{
			if (deltaTime < Duration::zero())
			{
				throw spk::Exception("RequestCoordinator cannot advance backwards");
			}
			_supervisor.forEachTimer([deltaTime](Timer &timer) {
				static_cast<ControlledTimer &>(timer).advance(deltaTime);
			});
		}

		void invalidate(const TKey &key)
		{
			auto it = _requests.find(key);
			if (it != _requests.end())
			{
				_invalidate(it->second);
				_requests.erase(it);
			}
		}

		template <typename TPredicate>
		void retain(TPredicate &&isWanted)
		{
			std::erase_if(_requests, [this, &isWanted](const auto &entry) {
				if (isWanted(entry.first))
				{
					return false;
				}
				_invalidate(entry.second);
				return true;
			});
		}

		void invalidateAll()
		{
			_supervisor.invalidateAll();
			_requests.clear();
		}

		template <typename TCallback>
		void collect(Duration now, TCallback &&onSuccess)
		{
			_now = now;
			_supervisor.collect([this, now, &onSuccess](Id id, const Answer &answer) {
				for (auto it = _requests.begin(); it != _requests.end(); ++it)
				{
					if (it->second.phase != Phase::Active || it->second.id != id)
					{
						continue;
					}
					const TKey key = it->first;
					if (answer.status() == Status::Failed)
					{
						_fail(key, it->second.generation, Failure::TaskFailed, now);
					}
					else
					{
						_requests.erase(it);
						if constexpr (std::is_void_v<TResult>)
						{
							onSuccess(key);
						}
						else
						{
							onSuccess(key, answer.result());
						}
					}
					return;
				}
			});
		}

		// A synchronous submission reserves budget too, including reentrant calls.
		template <typename TSubmit>
		[[nodiscard]] bool request(const TKey &key, Duration now, TSubmit &&submit)
		{
			if (inFlightCount() >= _maxInFlight)
			{
				return false;
			}

			auto it = _requests.find(key);
			if (it != _requests.end() && (it->second.phase != Phase::Waiting || now < it->second.retryAt))
			{
				return false;
			}
			if (_lastGeneration == std::numeric_limits<std::uint64_t>::max())
			{
				throw spk::Exception("RequestCoordinator generation identifier exhausted");
			}

			const std::uint64_t generation = ++_lastGeneration;
			_requests.insert_or_assign(key, Request{generation});
			++_submitting;
			bool submitting = true;
			try
			{
				Answer answer = submit();
				--_submitting;
				submitting = false;
				const Id id = _supervisor.watch(
					std::move(answer),
					std::make_unique<ControlledTimer>(_timeout),
					[this, key](Id timedOut) {
						_timeoutRequest(key, timedOut);
					});

				it = _requests.find(key);
				if (it != _requests.end() && it->second.generation == generation)
				{
					it->second.phase = Phase::Active;
					it->second.id = id;
				}
				else
				{
					_supervisor.invalidate(id);
				}
				return true;
			} catch (...)
			{
				if (submitting)
				{
					--_submitting;
				}
				_fail(key, generation, Failure::SubmissionFailed, now);
				return false;
			}
		}

		[[nodiscard]] std::size_t inFlightCount() const noexcept
		{
			return _supervisor.inFlightCount() + _submitting;
		}
	};
}
