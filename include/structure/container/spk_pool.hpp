#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "structure/system/spk_exception.hpp"

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
			std::mutex mutex;
			std::vector<std::unique_ptr<TType>> availableElements;
			size_t maximumSize = std::numeric_limits<size_t>::max();

			bool closed = false;
		};

		std::shared_ptr<Data> _data;

	public:
		Pool() :
			_data(std::make_shared<Data>())
		{
		}

		~Pool()
		{
			std::scoped_lock<std::mutex> lock(_data->mutex);
			_data->closed = true;
		}

		Pool(const Pool &) = delete;
		Pool &operator=(const Pool &) = delete;

		void setMaximumAllocationSize(size_t p_size)
		{
			std::scoped_lock<std::mutex> lock(_data->mutex);

			if (_data->closed == true)
			{
				GENERATE_ERROR("Can't edit the maximulm allocated size of a closed pool");
			}

			_data->maximumSize = p_size;

			while (_data->availableElements.size() > _data->maximumSize)
			{
				_data->availableElements.pop_back();
			}
		}

		size_t size() const
		{
			std::scoped_lock<std::mutex> lock(_data->mutex);
			return _data->availableElements.size();
		}

		template <typename... TArgs>
		void allocate(TArgs &&...args)
		{
			std::scoped_lock<std::mutex> lock(_data->mutex);

			if (_data->closed == true)
			{
				GENERATE_ERROR("Can't allocate a new object in closed pool");
			}

			_data->availableElements.emplace_back(std::make_unique<TType>(std::forward<TArgs>(args)...));
		}

		template <typename... TArgs>
		void resize(size_t p_newSize, TArgs &&...args)
		{
			std::scoped_lock<std::mutex> lock(_data->mutex);

			if (_data->closed == true)
			{
				GENERATE_ERROR("Can't resize a closed pool");
			}

			while (_data->availableElements.size() > p_newSize)
			{
				_data->availableElements.pop_back();
			}

			while (_data->availableElements.size() < p_newSize)
			{
				_data->availableElements.emplace_back(std::make_unique<TType>(std::forward<TArgs>(args)...));
			}
		}

		void release()
		{
			std::scoped_lock<std::mutex> lock(_data->mutex);

			if (_data->closed == true)
			{
				GENERATE_ERROR("Can't release a closed pool");
			}

			_data->availableElements.clear();
		}

		template <typename... TArgs>
		Object obtain(TArgs &&...args)
		{
			{
				std::scoped_lock<std::mutex> lock(_data->mutex);

				if (_data->closed == true)
				{
					GENERATE_ERROR("Can't obtain object from a closed pool");
				}
			}

			TType *rawPtr = nullptr;

			{
				std::scoped_lock<std::mutex> lock(_data->mutex);

				if (_data->availableElements.empty())
				{
					rawPtr = new TType(std::forward<TArgs>(args)...);
				}
				else
				{
					rawPtr = _data->availableElements.back().release();
					_data->availableElements.pop_back();

					rawPtr->~TType();
					new (rawPtr) TType(std::forward<TArgs>(args)...);
				}
			}

			std::weak_ptr<Data> weakData = _data;

			auto deleter = [weakData](TType *ptr)
			{
				if (auto weakDataContent = weakData.lock())
				{
					std::scoped_lock<std::mutex> lock(weakDataContent->mutex);

					if (weakDataContent->closed == false && weakDataContent->availableElements.size() < weakDataContent->maximumSize)
					{
						weakDataContent->availableElements.emplace_back(ptr);
						return;
					}
				}

				delete ptr;
			};

			return Object(rawPtr, deleter);
		}
	};
}
