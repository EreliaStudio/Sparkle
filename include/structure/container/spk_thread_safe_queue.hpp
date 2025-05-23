#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace spk
{
	template <typename TType>
	class ThreadSafeQueue
	{
	private:
		std::queue<TType> m_queue;
		std::mutex m_mutex;
		std::condition_variable m_cond;

	public:
		bool empty() const
		{
			return (m_queue.empty());
		}

		void push(TType &&item)
		{
			std::unique_lock<std::mutex> lock(m_mutex);

			m_queue.push(std::move(item));
			m_cond.notify_one();
		}

		TType pop()
		{
			std::unique_lock<std::mutex> lock(m_mutex);

			m_cond.wait(lock, [&]() { return !m_queue.empty(); });

			TType item = std::move(m_queue.front());
			m_queue.pop();

			return std::move(item);
		}
	};
}