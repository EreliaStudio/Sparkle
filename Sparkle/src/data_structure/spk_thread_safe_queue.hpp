#pragma once

#include <mutex>
#include <deque>
#include <condition_variable>

namespace spk
{
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
