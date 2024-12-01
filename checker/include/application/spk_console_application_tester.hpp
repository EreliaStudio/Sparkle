#pragma once

#include "application/spk_console_application.hpp"
#include "gtest/gtest.h"

class ConsoleApplicationTest : public ::testing::Test
{
protected:
	spk::ConsoleApplication app = spk::ConsoleApplication(L"ConsoleApplication");

	void SetUp() override
	{
		// Setup code if needed
	}

	void TearDown() override
	{
		// Teardown code if needed
	}
};