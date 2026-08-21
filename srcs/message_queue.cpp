#include "message_queue.hpp"

#include <Windows.h>

#include <system_error>

namespace spk::WinAPI
{
	void MessageQueue::dispatchPending() noexcept
	{
		MSG message{};
		while (::PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != FALSE)
		{
			::TranslateMessage(&message);
			::DispatchMessageW(&message);
		}
	}

	void MessageQueue::waitForActivity(HANDLE wakeEvent)
	{
		const DWORD result = ::MsgWaitForMultipleObjectsEx(
			1,
			&wakeEvent,
			INFINITE,
			QS_ALLINPUT,
			MWMO_INPUTAVAILABLE);

		if (result == WAIT_FAILED)
		{
			throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "MsgWaitForMultipleObjectsEx");
		}
	}
}
