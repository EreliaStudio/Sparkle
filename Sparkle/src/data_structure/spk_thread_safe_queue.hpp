#pragma once

#include <mutex>
#include <deque>
#include <condition_variable>

namespace spk
{
	/**
	 * @class ThreadSafeDeque
	 * @brief A thread-safe wrapper around std::deque, enabling safe access and modification across multiple threads.
	 *
	 * This template class provides synchronized access to a deque, ensuring that operations such as push, pop, and access
	 * are safe to call from different threads. It uses mutexes to lock the deque during modifications and provides a
	 * condition variable to facilitate waiting for new elements to be added when the deque is empty.
	 *
	 * Key functionalities include:
	 * - Thread-safe push and pop operations at both ends of the deque.
	 * - Access to the front and back elements with thread safety.
	 * - Ability to wait for new elements when the deque is empty, making it suitable for producer-consumer scenarios.
	 * - Size and empty checks, and a clear function to empty the deque.
	 *
	 * Usage example:
	 * @code
	 * spk::ThreadSafeDeque<int> myDeque;
	 * // Producer thread
	 * myDeque.push_back(42);
	 *
	 * // Consumer thread
	 * myDeque.wait(); // Wait for an element to be available
	 * auto value = myDeque.pop_front();
	 * @endcode
	 *
	 * @note This class deletes the copy constructor to prevent accidental copying, which is not safe for a thread-safe container.
	 * @note This class is expected to be re-done later on
	 */
	template <typename TType>
	class ThreadSafeDeque
	{
	protected:
		std::mutex muxQueue;
		std::mutex muxBlocking;

		std::deque<TType> _content;
		std::condition_variable cvBlocking;

	public:
		ThreadSafeDeque() = default;

		ThreadSafeDeque(const ThreadSafeDeque<TType> &) = delete;

		virtual ~ThreadSafeDeque() { clear(); }

		const TType &operator[](const size_t &p_index) const
		{
			return (_content[p_index]);
		}

		TType &front()
		{
			std::scoped_lock lock(muxQueue);
			return _content.front();
		}

		TType &back()
		{
			std::scoped_lock lock(muxQueue);
			return _content.back();
		}

		const TType &front() const
		{
			std::scoped_lock lock(muxQueue);
			return _content.front();
		}

		const TType &back() const
		{
			std::scoped_lock lock(muxQueue);
			return _content.back();
		}

		TType pop_front()
		{
			std::scoped_lock lock(muxQueue);
			auto t = std::move(_content.front());
			_content.pop_front();
			return (std::move(t));
		}

		TType pop_back()
		{
			std::scoped_lock lock(muxQueue);
			auto t = std::move(_content.back());
			_content.pop_back();
			return (std::move(t));
		}

		void push_back(TType &&item)
		{
			std::scoped_lock lock(muxQueue);
			_content.emplace_back(std::move(item));

			std::unique_lock<std::mutex> ul(muxBlocking);
			cvBlocking.notify_one();
		}

		void push_front(TType &&item)
		{
			std::scoped_lock lock(muxQueue);
			_content.emplace_front(std::move(item));

			std::unique_lock<std::mutex> ul(muxBlocking);
			cvBlocking.notify_one();
		}

		bool empty()
		{
			return _content.empty();
		}

		size_t size()
		{
			return _content.size();
		}

		void clear()
		{
			std::scoped_lock lock(muxQueue);
			_content.clear();
		}

		void wait()
		{
			if (empty())
			{
				std::unique_lock<std::mutex> ul(muxBlocking);
				cvBlocking.wait(ul);
			}
		}
	};
}
