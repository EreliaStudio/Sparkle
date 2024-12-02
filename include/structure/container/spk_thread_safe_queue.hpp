#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

namespace spk
{
	/**
	 * @class ThreadSafeQueue
	 * @brief A thread-safe queue implementation for concurrent access.
	 * 
	 * The `ThreadSafeQueue` class provides a safe way to push and pop elements
	 * in a multi-threaded environment. It uses a mutex for synchronization and
	 * a condition variable to manage blocking on `pop()` until an element is available.
	 * 
	 * Example usage:
	 * @code
	 * spk::ThreadSafeQueue<int> queue;
	 * 
	 * queue.push(42); // Add an element to the queue
	 * int value = queue.pop(); // Retrieve and remove the front element
	 * @endcode
	 * 
	 * @tparam TType The type of elements stored in the queue.
	 */
	template <typename TType>
	class ThreadSafeQueue
	{
	private:
		std::queue<TType> m_queue;
		mutable std::mutex m_mutex;
		std::condition_variable m_cond;

	public:
		/**
		 * @brief Checks if the queue is empty.
		 * 
		 * @return `true` if the queue is empty, otherwise `false`.
		 */
		bool empty() const
		{	
			std::unique_lock<std::mutex> lock(m_mutex); // Lock the mutex for thread-safe access
			return m_queue.empty();
		}

		/**
		 * @brief Pushes a new element to the back of the queue.
		 * 
		 * @param item The element to push. This method uses move semantics.
		 */
		void push(TType&& item)
		{
			std::unique_lock<std::mutex> lock(m_mutex); // Lock the mutex for thread-safe access

			m_queue.push(std::move(item)); // Move the item into the queue
			m_cond.notify_one(); // Notify one waiting thread that an item is available
		}

		/**
		 * @brief Removes and returns the front element from the queue.
		 * 
		 * This method blocks if the queue is empty until an element becomes available.
		 * 
		 * @return The front element from the queue.
		 */
		TType pop()
		{
			std::unique_lock<std::mutex> lock(m_mutex); // Lock the mutex for thread-safe access

			m_cond.wait(lock, [&]() { return !m_queue.empty(); }); // Wait until the queue is not empty

			TType item = m_queue.front(); // Get the front element
			m_queue.pop(); // Remove the front element

			return std::move(item); // Return the element
		}
	};
}
