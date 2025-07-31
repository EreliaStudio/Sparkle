#include "structure/thread/spk_thread.hpp"

namespace spk
{
	Thread::Thread(const std::wstring &p_name, const std::function<void()> &p_callback) :
			_name(p_name),
			_innerCallback(p_callback)
	{
	}

	Thread::~Thread()
	{
		if (isJoinable() == true)
		{
			join();
		}
	}

	void Thread::start()
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

	void Thread::join()
	{
		if (_handle != nullptr && _handle->joinable())
		{
			_handle->join();
			_handle.reset();
			_handle = nullptr;
		}
	}

	bool Thread::isJoinable() const
	{
		if (_handle == nullptr)
		{
			return (false);
		}
		return (_handle->joinable());
	}
}
