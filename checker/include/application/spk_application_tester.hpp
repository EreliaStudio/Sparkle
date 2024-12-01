#pragma once

#include "application/spk_application.hpp"
#include <gtest/gtest.h>

class ApplicationTest : public ::testing::Test
{
protected:
	spk::Application app;
	std::atomic<int> counter;
	spk::ContractProvider::Job callback;

	void SetUp() override
	{
		counter = 0;
		callback = [this]() { ++counter; };
	}
};