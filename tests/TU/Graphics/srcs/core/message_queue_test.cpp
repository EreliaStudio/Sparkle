#include <gtest/gtest.h>

#include <Windows.h>

#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

#include "core/platform/message_queue.hpp"
#include "core/platform/wake_event.hpp"
#include "core/platform/window.hpp"

namespace
{
	bool contains(std::string_view text, std::string_view fragment)
	{
		return text.find(fragment) != std::string_view::npos;
	}


	std::string uniqueIdentifier(std::string_view prefix)
	{
		static std::atomic_uint64_t counter = 0;
		return std::string(prefix) + std::to_string(::GetCurrentProcessId()) + "_" +
			std::to_string(::GetTickCount64()) + "_" + std::to_string(counter.fetch_add(1));
	}

	void ensureThreadQueueExists()
	{
		MSG message{};
		::PeekMessageW(&message, nullptr, 0, 0, PM_NOREMOVE);
	}
}

TEST(MessageQueueTest, StandardUsageWakesWithSuppliedEvent)
{
	spk::WinAPI::WakeEvent wake;
	std::promise<void> entered;
	std::promise<void> returned;
	std::thread waiter([&] {
		ensureThreadQueueExists();
		entered.set_value();
		spk::WinAPI::MessageQueue::waitForActivity(wake.handle());
		returned.set_value();
	});

	entered.get_future().wait();
	wake.notify();
	EXPECT_EQ(returned.get_future().wait_for(std::chrono::seconds(5)), std::future_status::ready);
	waiter.join();
}

TEST(MessageQueueTest, DispatchPendingWithNoMessagesIsNoOp)
{
	ensureThreadQueueExists();
	EXPECT_NO_THROW(spk::WinAPI::MessageQueue::dispatchPending());
}

TEST(MessageQueueTest, DispatchPendingConsumesPostedQuitMessage)
{
	ensureThreadQueueExists();
	::PostQuitMessage(73);
	spk::WinAPI::MessageQueue::dispatchPending();

	MSG message{};
	EXPECT_FALSE(::PeekMessageW(&message, nullptr, WM_QUIT, WM_QUIT, PM_NOREMOVE));
}

TEST(MessageQueueTest, DispatchPendingDispatchesPostedWindowMessage)
{
	constexpr UINT TestMessage = WM_APP + 70;
	spk::WinAPI::Window::Class windowClass(uniqueIdentifier("Sparkle_MessageQueue_"));
	spk::WinAPI::Window window;
	int dispatchCount = 0;

	spk::WinAPI::Window::CreationInfo creation;
	creation.title = "MessageQueueTest";
	creation.width = 160;
	creation.height = 90;
	creation.visible = false;
	creation.messageHandler = [&](HWND handle, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
		if (message == TestMessage)
		{
			++dispatchCount;
			return 0;
		}
		return ::DefWindowProcW(handle, message, wParam, lParam);
	};

	window.create(windowClass, creation);
	ASSERT_TRUE(::PostMessageW(window.handle(), TestMessage, 0, 0));
	spk::WinAPI::MessageQueue::dispatchPending();
	EXPECT_EQ(dispatchCount, 1);
	window.destroy();
}


TEST(MessageQueueTest, RepeatedWakeupsCanDriveRepeatedWaits)
{
	spk::WinAPI::WakeEvent wake;
	std::promise<void> entered;
	std::promise<void> firstReturned;
	std::promise<void> secondReturned;
	std::promise<void> continueToSecond;
	std::shared_future<void> continueFuture = continueToSecond.get_future().share();

	std::thread waiter([&] {
		ensureThreadQueueExists();
		entered.set_value();
		spk::WinAPI::MessageQueue::waitForActivity(wake.handle());
		firstReturned.set_value();
		continueFuture.wait();
		spk::WinAPI::MessageQueue::waitForActivity(wake.handle());
		secondReturned.set_value();
	});

	entered.get_future().wait();
	wake.notify();
	EXPECT_EQ(firstReturned.get_future().wait_for(std::chrono::seconds(5)), std::future_status::ready);
	continueToSecond.set_value();
	wake.notify();
	EXPECT_EQ(secondReturned.get_future().wait_for(std::chrono::seconds(5)), std::future_status::ready);
	waiter.join();
}

TEST(MessageQueueTest, PostedMessagesRemainThreadAffine)
{
	constexpr UINT TestMessage = WM_APP + 71;
	std::promise<DWORD> threadIdPromise;
	std::promise<void> inspectPromise;
	std::shared_future<void> inspectFuture = inspectPromise.get_future().share();
	std::promise<bool> foundPromise;

	std::thread worker([&] {
		ensureThreadQueueExists();
		threadIdPromise.set_value(::GetCurrentThreadId());
		inspectFuture.wait();
		MSG message{};
		const BOOL found = ::PeekMessageW(&message, nullptr, TestMessage, TestMessage, PM_REMOVE);
		foundPromise.set_value(found != FALSE && message.message == TestMessage);
	});

	const DWORD workerId = threadIdPromise.get_future().get();
	ASSERT_TRUE(::PostThreadMessageW(workerId, TestMessage, 123, 456));
	spk::WinAPI::MessageQueue::dispatchPending();
	inspectPromise.set_value();
	EXPECT_TRUE(foundPromise.get_future().get());
	worker.join();
}

TEST(MessageQueueTest, MessageActivityWakesTheOwningThread)
{
	constexpr UINT TestMessage = WM_APP + 72;
	spk::WinAPI::WakeEvent wake;
	std::promise<DWORD> threadIdPromise;
	std::promise<UINT> messagePromise;

	std::thread worker([&] {
		ensureThreadQueueExists();
		threadIdPromise.set_value(::GetCurrentThreadId());
		spk::WinAPI::MessageQueue::waitForActivity(wake.handle());
		MSG message{};
		const BOOL found = ::PeekMessageW(&message, nullptr, TestMessage, TestMessage, PM_REMOVE);
		messagePromise.set_value(found ? message.message : 0u);
	});

	const DWORD workerId = threadIdPromise.get_future().get();
	ASSERT_TRUE(::PostThreadMessageW(workerId, TestMessage, 0, 0));
	EXPECT_EQ(messagePromise.get_future().get(), TestMessage);
	worker.join();
}

TEST(MessageQueueTest, InvalidWaitHandleThrowsSystemError)
{
	try
	{
		spk::WinAPI::MessageQueue::waitForActivity(INVALID_HANDLE_VALUE);
		FAIL() << "Expected std::system_error";
	}
	catch (const std::system_error &exception)
	{
		EXPECT_EQ(exception.code().value(), ERROR_INVALID_HANDLE);
		EXPECT_TRUE(contains(exception.what(), "MsgWaitForMultipleObjectsEx"));
	}
}
