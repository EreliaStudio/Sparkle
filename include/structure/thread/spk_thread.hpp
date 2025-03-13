#pragma once

#include "structure/spk_iostream.hpp"

#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

namespace spk
{
	class Thread
	{
	private:
		std::wstring _name;
		std::function<void()> _innerCallback;
		std::unique_ptr<std::thread> _handle = nullptr;

	public:
		Thread(const std::wstring &p_name, const std::function<void()> &p_callback) :
			_name(p_name),
			_innerCallback(p_callback)
		{
		}

		~Thread()
		{
			if (isJoinable())
			{
				join();
			}
		}

		virtual void start()
		{
			if (_handle != nullptr)
			{
				join();
			}
			_handle = std::make_unique<std::thread>(
				[&]()
				{
					spk::cout.setPrefix(_name);
					_innerCallback();
				});
		}

		virtual void join()
		{
			if (_handle != nullptr && _handle->joinable())
			{
				_handle->join();
				_handle.reset();
				_handle = nullptr;
			}
		}

		bool isJoinable() const
		{
			if (_handle == nullptr)
			{
				return (false);
			}
			return (_handle->joinable());
		}
	};
}