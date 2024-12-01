#pragma once

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "structure/thread/spk_thread.hpp"

class ThreadTest : public ::testing::Test
{
protected:
	std::atomic<bool> callbackExecuted;
	std::wstring threadName;
	std::function<void()> callback;

	void SetUp() override
	{
		callbackExecuted = false;
		threadName = L"TestThread";
		callback = [this]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			callbackExecuted = true;
			};
	}
};