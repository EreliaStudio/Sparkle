#pragma once

#include <deque>
#include <memory>
#include <functional>
#include <mutex>
#include <utility>

namespace spk
{
	template <typename TType>
	class Pool : public std::enable_shared_from_this<Pool<TType>>
	{
	public:
		using Destructor = std::function<void(TType *)>;
		using Object = std::unique_ptr<TType, Destructor>;

	private:
		mutable std::mutex _mutex;

		size_t _preallocationLimit = std::numeric_limits<size_t>::max();
		std::deque<std::unique_ptr<TType>> _preallocatedElements;

		void _recycle(TType *ptr)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			if (_preallocatedElements.size() < _preallocationLimit)
			{
				_preallocatedElements.emplace_back(ptr);
			}
			else
			{
				delete ptr;
			}
		}

		void _insert(TType *ptr)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			std::unique_ptr<TType> bindedPtr = std::unique_ptr<TType>(ptr);

			if (_preallocatedElements.size() < _preallocationLimit)
			{
				_preallocatedElements.emplace_back(std::move(bindedPtr));
			}
		}

	public:
		Pool() = default;
		~Pool() = default;

		void setMaximumAllocatedSize(const size_t &p_size)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_preallocationLimit = p_size;

			while (_preallocatedElements.size() > _preallocationLimit)
			{
				_preallocatedElements.pop_back();
			}
		}

		size_t size() const
		{
			std::lock_guard<std::mutex> lock(_mutex);
			return _preallocatedElements.size();
		}

		template <typename... TArgs>
		void allocate(TArgs &&...args)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_insert(new TType(std::forward<TArgs>(args)...));
		}

		template <typename... TArgs>
		void resize(size_t p_newSize, TArgs &&...args)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			while (_preallocatedElements.size() < p_newSize)
			{
				_insert(new TType(std::forward<TArgs>(args)...));
			}
		}

		template <typename... TArgs>
		Object obtain(TArgs &&...args)
		{
			TType *tmpPointer;

			{
				std::lock_guard<std::mutex> lock(_mutex);
				if (_preallocatedElements.empty())
				{
					tmpPointer = new TType(std::forward<TArgs>(args)...);
				}
				else
				{
					tmpPointer = _preallocatedElements.front().release();

					_preallocatedElements.pop_front();

					tmpPointer->~TType();
					new (tmpPointer) TType(std::forward<TArgs>(args)...);
				}
			}

			std::weak_ptr<Pool<TType>> selfWeak = this->shared_from_this();
			auto deleter = [selfWeak](TType *ptr)
			{
				if (auto selfShared = selfWeak.lock())
				{
					selfShared->_recycle(p);
				}
				else
				{
					delete p;
				}
			};

			return (Object(tmpPointer, deleter));
		}
	};
}
