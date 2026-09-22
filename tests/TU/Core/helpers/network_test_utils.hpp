#pragma once

#include "container/thread_safe_fifo.hpp"

#include <chrono>
#include <exception>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace NetworkTestUtils
{
	using namespace std::chrono_literals;

	template <typename TPredicate>
	[[nodiscard]] bool waitUntil(
		TPredicate predicate,
		std::chrono::milliseconds timeout = 2s)
	{
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (std::chrono::steady_clock::now() < deadline)
		{
			if (predicate())
			{
				return true;
			}
			std::this_thread::sleep_for(2ms);
		}
		return predicate();
	}

	template <typename TValue>
	[[nodiscard]] std::vector<TValue> collect(
		spk::ThreadSafeFIFO<TValue> &queue,
		std::size_t expectedCount,
		std::chrono::milliseconds timeout = 2s)
	{
		std::vector<TValue> result;
		std::vector<TValue> batch;
		(void)waitUntil(
			[&] {
				queue.drain(batch);
				for (TValue &value : batch)
				{
					result.push_back(std::move(value));
				}
				return result.size() >= expectedCount;
			},
			timeout);
		return result;
	}

	class ThreadFailure
	{
	private:
		mutable std::mutex _mutex;
		std::exception_ptr _exception;

	public:
		template <typename TFunction>
		void run(TFunction function) noexcept
		{
			try
			{
				function();
			} catch (...)
			{
				const std::scoped_lock lock(_mutex);
				if (_exception == nullptr)
				{
					_exception = std::current_exception();
				}
			}
		}

		void rethrow() const
		{
			const std::scoped_lock lock(_mutex);
			if (_exception != nullptr)
			{
				std::rethrow_exception(_exception);
			}
		}
	};
}
