#pragma once

#include "structure/thread/spk_persistant_worker.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <atomic>

class PersistantWorkerTest : public ::testing::Test
{
protected:
	std::atomic<int> counter;
	std::wstring workerName;
	std::function<void()> callback;

	void SetUp() override
	{
		counter = 0;
		workerName = L"TestWorker";
		callback = [this]() {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				counter++;
			};
	}
};