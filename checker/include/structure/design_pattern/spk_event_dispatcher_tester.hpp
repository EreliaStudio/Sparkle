#pragma once

#include <gtest/gtest.h>

#include "structure/design_pattern/spk_event_dispatcher.hpp"
#include "structure/design_pattern/spk_singleton.hpp"

enum class TestEventType
{
	Number,
	Void,
	Mixed
};

class EventDispatcherTest : public ::testing::Test
{
protected:
	void TearDown() override
	{
		spk::Singleton<spk::EventDispatcher<TestEventType>>::release();
	}
};
