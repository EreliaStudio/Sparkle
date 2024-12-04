#pragma once

#include <gtest/gtest.h>

#include "structure/design_pattern/spk_state_machine.hpp"

struct RecordingStep : public spk::StateMachine::Step
{
	explicit RecordingStep(const spk::StateMachine::Step::ID &p_next = L"") :
		next(p_next)
	{
	}

	void onStart() override
	{
		startCount++;
	}

	void onPending() override
	{
		pendingCount++;
		if (autoFinish == true)
		{
			finish();
		}
	}

	spk::StateMachine::Step::ID onFinish() override
	{
		finishCount++;
		return next;
	}

	spk::StateMachine::Step::ID next;
	int startCount = 0;
	int pendingCount = 0;
	int finishCount = 0;
	bool autoFinish = false;
};

class StateMachineTest : public ::testing::Test
{
protected:
	spk::StateMachine machine;
};
