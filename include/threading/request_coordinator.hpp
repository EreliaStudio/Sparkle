#pragma once

#include <chrono>
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
	// Owns only state associated with submitted keys. There is no pending
	// queue: callers walk their own priority plan and offer eligible keys.
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

		// Returning nullopt disables retry until the key is invalidated.
		using RetryPolicy = std::function<std::optional<Duration>(const TKey &, Failure)>;

	private:
		struct Request
		{
			std::uint64_t generation = 0;
			std::optional<Id> active;
			std::optional<Duration> retryAt;
			bool submitting = false;
			bool disabled = false;
		};

		std::map<TKey, Request> _requests;
		Supervisor _supervisor;
		std::size_t _maxInFlight;
		std::size_t _submitting = 0;
		Duration _timeout;
		Duration _now = Duration::zero();
		RetryPolicy _retryPolicy;
		std::uint64_t _lastGeneration = 0;

		void _defer(const TKey &key, std::uint64_t generation, Failure reason, Duration now)
		{
			auto it = _requests.find(key);
			if (it == _requests.end() || it->second.generation != generation)
			{
				return;
			}

			it->second.active.reset();
			it->second.submitting = false;
			it->second.disabled = true;
			it->second.retryAt.reset();

			const std::optional<Duration> delay = _retryPolicy ? _retryPolicy(key, reason) : std::nullopt;
			it = _requests.find(key);
			if (it == _requests.end() || it->second.generation != generation)
			{
				return;
			}
			if (delay.has_value())
			{
				if (*delay < Duration::zero())
				{
					throw spk::Exception("RequestCoordinator retry delay cannot be negative");
				}
				it->second.retryAt = now + *delay;
				it->second.disabled = false;
			}
		}

		void _timeoutRequest(const TKey &key, Id id)
		{
			const auto it = _requests.find(key);
			if (it != _requests.end() && it->second.active == id)
			{
				_defer(key, it->second.generation, Failure::Timeout, _now);
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
				if (it->second.active.has_value())
				{
					_supervisor.invalidate(*it->second.active);
				}
				_requests.erase(it);
			}
		}

		template <typename TPredicate>
		void retain(TPredicate &&isWanted)
		{
			for (auto it = _requests.begin(); it != _requests.end();)
			{
				if (!isWanted(it->first))
				{
					if (it->second.active.has_value())
					{
						_supervisor.invalidate(*it->second.active);
					}
					it = _requests.erase(it);
				}
				else
				{
					++it;
				}
			}
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
					if (it->second.active != id)
					{
						continue;
					}
					if (answer.status() == Status::Failed)
					{
						const TKey key = it->first;
						_defer(key, it->second.generation, Failure::TaskFailed, now);
					}
					else
					{
						const TKey key = it->first;
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

		// Returns true only when a physical task has been admitted.
		// Submission exceptions are handled as a policy-driven failure.
		template <typename TSubmit>
		[[nodiscard]] bool request(const TKey &key, Duration now, TSubmit &&submit)
		{
			if (_supervisor.inFlightCount() + _submitting >= _maxInFlight)
			{
				return false;
			}

			auto [it, inserted] = _requests.try_emplace(key);
			(void)inserted;
			Request &record = it->second;
			if (record.active.has_value() || record.submitting || record.disabled ||
				(record.retryAt.has_value() && now < *record.retryAt))
			{
				return false;
			}

			if (_lastGeneration == std::numeric_limits<std::uint64_t>::max())
			{
				throw spk::Exception("RequestCoordinator generation identifier exhausted");
			}

			const std::uint64_t generation = ++_lastGeneration;
			record.generation = generation;
			record.submitting = true;
			record.retryAt.reset();
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
				if (it != _requests.end() && it->second.generation == generation && it->second.submitting)
				{
					it->second.active = id;
					it->second.submitting = false;
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
				_defer(key, generation, Failure::SubmissionFailed, now);
				return false;
			}
		}

		[[nodiscard]] std::size_t inFlightCount() const noexcept
		{
			return _supervisor.inFlightCount() + _submitting;
		}
	};
}
