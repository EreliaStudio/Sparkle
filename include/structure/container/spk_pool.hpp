#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <utility>
#include <limits>

namespace spk
{
	template <typename TType>
	class Pool
	{
	public:
		using Deleter = std::function<void(TType *)>;
		using Object = std::unique_ptr<TType, Deleter>;

	private:
		struct Data
		{
			std::mutex _mutex;
			std::vector<std::unique_ptr<TType>> _availableElements;
			size_t _maximumSize = std::numeric_limits<size_t>::max();

			bool _closed = false;
		};

		std::shared_ptr<Data> _data;

	public:
		Pool() :
			_data(std::make_shared<Data>())
		{
		}

		~Pool()
		{
			std::scoped_lock<std::mutex> lock(_data->_mutex);
			_data->_closed = true;
		}

		Pool(const Pool &) = delete;
		Pool &operator=(const Pool &) = delete;

		void setMaximumAllocationSize(size_t p_size)
		{
			std::scoped_lock<std::mutex> lock(_data->_mutex);

			if (_data->_closed == true)
			{
				throw std::runtime_error("Can't edit the maximulm allocated size of a closed pool");
			}

			_data->_maximumSize = p_size;

			while (_data->_availableElements.size() > _data->_maximumSize)
			{
				_data->_availableElements.pop_back();
			}
		}

		size_t size() const
		{
			std::scoped_lock<std::mutex> lock(_data->_mutex);
			return _data->_availableElements.size();
		}

		template <typename... TArgs>
		void allocate(TArgs &&...args)
		{
			std::scoped_lock<std::mutex> lock(_data->_mutex);

			if (_data->_closed == true)
			{
				throw std::runtime_error("Can't allocate a new object in closed pool");
			}

			_data->_availableElements.emplace_back(
				std::make_unique<TType>(std::forward<TArgs>(args)...));
		}

		template <typename... TArgs>
		void resize(size_t p_newSize, TArgs &&...args)
		{
			std::scoped_lock<std::mutex> lock(_data->_mutex);

			if (_data->_closed == true)
			{
				throw std::runtime_error("Can't resize a closed pool");
			}

			while (_data->_availableElements.size() > p_newSize)
			{
				_data->_availableElements.pop_back();
			}

			while (_data->_availableElements.size() < p_newSize)
			{
				_data->_availableElements.emplace_back(
					std::make_unique<TType>(std::forward<TArgs>(args)...));
			}
		}

		void release()
		{
			std::scoped_lock<std::mutex> lock(_data->_mutex);

			if (_data->_closed == true)
			{
				throw std::runtime_error("Can't release a closed pool");
			}

			_data->_availableElements.clear();
		}

		template <typename... TArgs>
		Object obtain(TArgs &&...args)
		{
			{
				std::scoped_lock<std::mutex> lock(_data->_mutex);

				if (_data->_closed == true)
				{
					throw std::runtime_error("Can't obtain object from a closed pool");
				}
			}

			TType *rawPtr = nullptr;

			{
				std::scoped_lock<std::mutex> lock(_data->_mutex);

				if (_data->_availableElements.empty())
				{
					rawPtr = new TType(std::forward<TArgs>(args)...);
				}
				else
				{
					rawPtr = _data->_availableElements.back().release();
					_data->_availableElements.pop_back();

					rawPtr->~TType();
					new (rawPtr) TType(std::forward<TArgs>(args)...);
				}
			}

			std::weak_ptr<Data> weakData = _data;

			auto deleter = [weakData](TType *ptr)
			{
				if (auto data = weakData.lock())
				{
					std::scoped_lock<std::mutex> lock(data->_mutex);

					if (data->_closed == false &&
						data->_availableElements.size() < data->_maximumSize)
					{
						data->_availableElements.emplace_back(ptr);
						return;
					}
				}

				delete ptr;
			};

			return Object(rawPtr, deleter);
		}
	};
}
