#pragma once

#include <Windows.h>

#include <stdexcept>
#include <system_error>

namespace spk::WinAPI
{
	class WakeEvent final
	{
	public:
		using NotifyOperation = BOOL(WINAPI *)(HANDLE);
		using CreateOperation = HANDLE(WINAPI *)(LPSECURITY_ATTRIBUTES, BOOL, BOOL, LPCWSTR);

	private:
		HANDLE _handle = nullptr;
		NotifyOperation _notifyOperation;

	public:
		explicit WakeEvent(
			NotifyOperation notifyOperation = ::SetEvent,
			CreateOperation createOperation = ::CreateEventW) :
			_notifyOperation(notifyOperation)
		{
			if (_notifyOperation == nullptr)
			{
				throw std::invalid_argument("WakeEvent notify operation cannot be null");
			}
			if (createOperation == nullptr)
			{
				throw std::invalid_argument("WakeEvent create operation cannot be null");
			}
			_handle = createOperation(nullptr, FALSE, FALSE, nullptr);
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
			if (_notifyOperation(_handle) == FALSE)
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
