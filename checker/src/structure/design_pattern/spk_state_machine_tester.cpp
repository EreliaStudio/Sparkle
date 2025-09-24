#include "structure/design_pattern/spk_state_machine_tester.hpp"

#include <memory>
#include <stdexcept>

TEST_F(StateMachineTest, AddAndContainsSteps)
{
	auto step = std::make_unique<RecordingStep>();
	machine.addStep(L"idle", std::move(step));

	ASSERT_EQ(machine.contains(L"idle"), true);
	ASSERT_EQ(machine.isActive(), false);
}

TEST_F(StateMachineTest, SetStepActivatesAndResets)
{
	auto step = std::make_unique<RecordingStep>();
	RecordingStep *stepPtr = step.get();

	machine.addStep(L"start", std::move(step));
	machine.setStep(L"start");

	ASSERT_EQ(machine.isActive(), true);
	ASSERT_EQ(machine.currentStepID(), L"start");
	ASSERT_EQ(stepPtr->isStarted(), false);
	ASSERT_EQ(stepPtr->isFinished(), false);
}

TEST_F(StateMachineTest, UpdateInvokesLifecycle)
{
	auto step = std::make_unique<RecordingStep>();
	RecordingStep *stepPtr = step.get();

	machine.addStep(L"loop", std::move(step));
	machine.setStep(L"loop");

	machine.update();
	ASSERT_EQ(stepPtr->startCount, 1);
	ASSERT_EQ(stepPtr->pendingCount, 1);
	ASSERT_EQ(stepPtr->finishCount, 0);

	machine.update();
	ASSERT_EQ(stepPtr->startCount, 1);
	ASSERT_EQ(stepPtr->pendingCount, 2);
	ASSERT_EQ(stepPtr->finishCount, 0);
}

TEST_F(StateMachineTest, TransitionOnFinish)
{
	auto first = std::make_unique<RecordingStep>(L"second");
	RecordingStep *firstPtr = first.get();
	auto second = std::make_unique<RecordingStep>();
	RecordingStep *secondPtr = second.get();

	machine.addStep(L"first", std::move(first));
	machine.addStep(L"second", std::move(second));

	machine.setStep(L"first");

	firstPtr->autoFinish = true;
	machine.update();

	ASSERT_EQ(machine.currentStepID(), L"second");
	ASSERT_EQ(firstPtr->finishCount, 1);
	ASSERT_EQ(secondPtr->startCount, 1);
}

TEST_F(StateMachineTest, ResetStepClearsFlags)
{
	auto step = std::make_unique<RecordingStep>();
	RecordingStep *stepPtr = step.get();

	machine.addStep(L"work", std::move(step));
	machine.setStep(L"work");

	machine.update();
	ASSERT_EQ(stepPtr->isStarted(), true);

	machine.resetStep();
	ASSERT_EQ(stepPtr->isStarted(), false);
	ASSERT_EQ(stepPtr->isFinished(), false);
}

TEST_F(StateMachineTest, StopClearsCurrentStep)
{
	auto step = std::make_unique<RecordingStep>();
	machine.addStep(L"processing", std::move(step));
	machine.setStep(L"processing");

	ASSERT_EQ(machine.isActive(), true);

	machine.stop();
	ASSERT_EQ(machine.isActive(), false);
	EXPECT_THROW(machine.currentStepID(), std::runtime_error);
}

TEST_F(StateMachineTest, ClearRemovesAllSteps)
{
	auto step = std::make_unique<RecordingStep>();
	machine.addStep(L"cleanup", std::move(step));

	machine.clear();
	ASSERT_EQ(machine.contains(L"cleanup"), false);
	ASSERT_EQ(machine.isActive(), false);
}

TEST_F(StateMachineTest, SettingUnknownStepThrows)
{
	EXPECT_THROW(machine.setStep(L"unknown"), std::runtime_error);
}

TEST_F(StateMachineTest, CurrentStepWithoutSelectionThrows)
{
	EXPECT_THROW(machine.currentStepID(), std::runtime_error);
}

TEST_F(StateMachineTest, DefaultStepCallbacksAreInvoked)
{
	bool started = false;
	bool pending = false;
	bool finished = false;

	spk::DefaultStep step(
		[&started]() { started = true; },
		[&pending]() { pending = true; },
		[&finished]()
		{
			finished = true;
			return L"";
		});

	step.onStart();
	step.onPending();
	step.finish();
	auto next = step.onFinish();

	ASSERT_EQ(started, true);
	ASSERT_EQ(pending, true);
	ASSERT_EQ(finished, true);
	ASSERT_EQ(next, L"");
}
