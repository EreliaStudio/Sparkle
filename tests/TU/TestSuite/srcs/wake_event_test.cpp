#include <gtest/gtest.h>

#include <Windows.h>

#include <atomic>
#include <future>
#include <string>
#include <system_error>
#include <thread>
#include <type_traits>

#include "core/platform/wake_event.hpp"

namespace
{
	bool contains(std::string_view text, std::string_view fragment)
	{
		return text.find(fragment) != std::string_view::npos;
	}
}

static_assert(!std::is_copy_constructible_v<spk::WinAPI::WakeEvent>);
static_assert(!std::is_move_constructible_v<spk::WinAPI::WakeEvent>);
static_assert(!std::is_copy_assignable_v<spk::WinAPI::WakeEvent>);
static_assert(!std::is_move_assignable_v<spk::WinAPI::WakeEvent>);

TEST(WakeEventTest, StandardUsage)
{
	spk::WinAPI::WakeEvent event;
	ASSERT_NE(event.handle(), nullptr);
	EXPECT_NE(event.handle(), INVALID_HANDLE_VALUE);

	std::promise<DWORD> resultPromise;
	std::thread waiter([&] {
		resultPromise.set_value(::WaitForSingleObject(event.handle(), 5000));
	});

	event.notify();
	EXPECT_EQ(resultPromise.get_future().get(), WAIT_OBJECT_0);
	waiter.join();

	EXPECT_EQ(::WaitForSingleObject(event.handle(), 0), WAIT_TIMEOUT);
}

TEST(WakeEventTest, AutoResetRequiresAnotherNotification)
{
	spk::WinAPI::WakeEvent event;
	event.notify();
	EXPECT_EQ(::WaitForSingleObject(event.handle(), 1000), WAIT_OBJECT_0);
	EXPECT_EQ(::WaitForSingleObject(event.handle(), 0), WAIT_TIMEOUT);

	event.notify();
	EXPECT_EQ(::WaitForSingleObject(event.handle(), 1000), WAIT_OBJECT_0);
	EXPECT_EQ(::WaitForSingleObject(event.handle(), 0), WAIT_TIMEOUT);
}

TEST(WakeEventTest, SetEventFailureReportsCodeAndOperation)
{
	spk::WinAPI::WakeEvent event;
	const HANDLE handle = event.handle();
	ASSERT_TRUE(::CloseHandle(handle));

	try
	{
		event.notify();
		FAIL() << "Expected std::system_error";
	}
	catch (const std::system_error &exception)
	{
		EXPECT_EQ(exception.code().value(), ERROR_INVALID_HANDLE);
		EXPECT_TRUE(contains(exception.what(), "SetEvent"));
	}
}

// Disabled: CreateEventW failure cannot be forced deterministically through WakeEvent's
// public API. Add an injectable Win32 creation seam before enabling this contract test.
TEST(WakeEventTest, DISABLED_CreateEventFailureReportsCodeAndOperation)
{
	FAIL() << "Requires a deterministic CreateEventW failure seam";
}
