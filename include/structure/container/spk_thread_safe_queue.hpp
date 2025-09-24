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
		std::queue<TType> _mQueue;
		std::mutex _mMutex;
		std::condition_variable _mCond;

	public:
		bool empty() const
		{
			return (_mQueue.empty());
		}

		void push(const TType &p_item)
		{
			std::unique_lock<std::mutex> lock(_mMutex);

			_mQueue.push(p_item);
			_mCond.notify_one();
		}

		void push(TType &&p_item)
		{
			std::unique_lock<std::mutex> lock(_mMutex);

			_mQueue.push(std::move(p_item));
			_mCond.notify_one();
		}

		TType pop()
		{
			std::unique_lock<std::mutex> lock(_mMutex);

			_mCond.wait(lock, [&]() { return !_mQueue.empty(); });

			TType item = std::move(_mQueue.front());
			_mQueue.pop();

			return std::move(item);
		}
	};
}