#pragma once

#include <Windows.h>

#include <system_error>

namespace spk::WinAPI
{
	class WakeEvent final
	{
	private:
		HANDLE _handle = nullptr;

	public:
		WakeEvent()
		{
			_handle = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
			if (_handle == nullptr)
			{
				throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "CreateEventW");
			}
		}

		WakeEvent(const WakeEvent &) = delete;
		WakeEvent(WakeEvent &&) = delete;
		~WakeEvent()
		{
			::CloseHandle(_handle);
		}

		WakeEvent &operator=(const WakeEvent &) = delete;
		WakeEvent &operator=(WakeEvent &&) = delete;

		void notify() const
		{
			if (::SetEvent(_handle) == FALSE)
			{
				throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "SetEvent");
			}
		}

		[[nodiscard]] HANDLE handle() const noexcept
		{
			return _handle;
		}
	};
}