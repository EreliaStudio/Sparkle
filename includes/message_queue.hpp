#pragma once

#include <Windows.h>

namespace spk::WinAPI
{
	class MessageQueue final
	{
	public:
		static void dispatchPending() noexcept;
		static void waitForActivity(HANDLE wakeEvent);
	};
}
