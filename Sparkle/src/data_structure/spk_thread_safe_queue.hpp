#pragma once

#include <mutex>
#include <deque>
#include <condition_variable>

namespace spk
{
	/**
	 * @class ThreadSafeDeque
	 *
	 * @tparam TType The type of data to be managed by the ThreadSafeQueue.
	 *
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
	private:
		std::mutex muxQueue;
		std::mutex muxBlocking;

		std::deque<TType> _content;
		std::condition_variable cvBlocking;

	public:
		/**
		 * @brief Default constructor.
		 *
		 * Initializes a new empty instance of the ThreadSafeDeque class.
		 */
		ThreadSafeDeque() = default;

		/**
		 * @brief Deleted copy constructor.
		 *
		 * Prevents copying of ThreadSafeDeque instances to ensure thread safety.
		 */
		ThreadSafeDeque(const ThreadSafeDeque<TType> &) = delete;

		/**
		 * @brief Destructor.
		 *
		 * Clears the deque contents safely upon destruction.
		 */
		virtual ~ThreadSafeDeque() { clear(); }

		/**
		 * @brief Access operator.
		 *
		 * Provides const access to elements at the specified index in the deque.
		 *
		 * @param p_index Index of the element to access.
		 * @return A const reference to the element at the specified index.
		 */
		const TType &operator[](const size_t &p_index) const
		{
			return (_content[p_index]);
		}

		/**
		 * @brief Gets the front element.
		 *
		 * Provides mutable access to the first element in the deque.
		 *
		 * @return A reference to the front element.
		 */
		TType &front()
		{
			std::scoped_lock lock(muxQueue);
			return _content.front();
		}

		/**
		 * @brief Gets the back element.
		 *
		 * Provides mutable access to the last element in the deque.
		 *
		 * @return A reference to the back element.
		 */
		TType &back()
		{
			std::scoped_lock lock(muxQueue);
			return _content.back();
		}

		/**
		 * @brief Gets the front element (const).
		 *
		 * Provides const access to the first element in the deque.
		 *
		 * @return A const reference to the front element.
		 */
		const TType &front() const
		{
			std::scoped_lock lock(muxQueue);
			return _content.front();
		}

		/**
		 * @brief Gets the back element (const).
		 *
		 * Provides const access to the last element in the deque.
		 *
		 * @return A const reference to the back element.
		 */
		const TType &back() const
		{
			std::scoped_lock lock(muxQueue);
			return _content.back();
		}

		/**
		 * @brief Removes and returns the front element.
		 *
		 * Safely removes the first element from the deque and returns it.
		 *
		 * @return The removed front element.
		 */
		TType pop_front()
		{
			std::scoped_lock lock(muxQueue);
			auto t = std::move(_content.front());
			_content.pop_front();
			return (std::move(t));
		}

		/**
		 * @brief Removes and returns the back element.
		 *
		 * Safely removes the last element from the deque and returns it.
		 *
		 * @return The removed back element.
		 */
		TType pop_back()
		{
			std::scoped_lock lock(muxQueue);
			auto t = std::move(_content.back());
			_content.pop_back();
			return (std::move(t));
		}

		/**
		 * @brief Adds an element to the back.
		 *
		 * Safely adds a new element to the end of the deque.
		 *
		 * @param item The element to add.
		 */
		void push_back(TType &&item)
		{
			std::scoped_lock lock(muxQueue);
			_content.emplace_back(std::move(item));

			std::unique_lock<std::mutex> ul(muxBlocking);
			cvBlocking.notify_one();
		}

		/**
		 * @brief Adds an element to the front.
		 *
		 * Safely adds a new element to the beginning of the deque.
		 *
		 * @param item The element to add.
		 */
		void push_front(TType &&item)
		{
			std::scoped_lock lock(muxQueue);
			_content.emplace_front(std::move(item));

			std::unique_lock<std::mutex> ul(muxBlocking);
			cvBlocking.notify_one();
		}

		/**
		 * @brief Checks if the deque is empty.
		 *
		 * @return True if the deque is empty, false otherwise.
		 */
		bool empty()
		{
			return _content.empty();
		}

		/**
		 * @brief Gets the size of the deque.
		 *
		 * @return The number of elements in the deque.
		 */
		size_t size()
		{
			return _content.size();
		}

		/**
		 * @brief Clears the deque.
		 *
		 * Safely removes all elements from the deque.
		 */
		void clear()
		{
			std::scoped_lock lock(muxQueue);
			_content.clear();
		}

		/**
		 * @brief Waits for the deque to have elements.
		 *
		 * Blocks the calling thread until at least one element is available in the deque.
		 */
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
