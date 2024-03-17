#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "design_pattern/spk_state_machine.hpp"

using namespace spk;

enum class TestState {
    Idle,
    Running,
    Stopping
};

class StateMachineTest : public ::testing::Test {
protected:
    StateMachine<TestState> stateMachine;
    int enterCount = 0;
    int updateCount = 0;
    int exitCount = 0;

    virtual void SetUp() override {
        stateMachine.addState(TestState::Idle, {
            [this] { enterCount++; },  // onEnter
            [this] { updateCount++; }, // onUpdate
            [this] { exitCount++; }    // onExit
        });

        stateMachine.addState(TestState::Running, {
            [this] { enterCount++; },
            [this] { updateCount++; },
            [this] { exitCount++; }
        });

        stateMachine.addState(TestState::Stopping, {
            [this] { enterCount++; },
            [this] { updateCount++; },
            [this] { exitCount++; }
        });

		enterCount = 0;
		updateCount = 0;
		exitCount = 0;
    }
};

TEST_F(StateMachineTest, EnterState) {
    stateMachine.enterState(TestState::Idle);
    ASSERT_EQ(enterCount, 1);
    ASSERT_EQ(stateMachine.state(), TestState::Idle);
}

TEST_F(StateMachineTest, UpdateState) {
    stateMachine.enterState(TestState::Idle);
    stateMachine.update();
    ASSERT_EQ(updateCount, 1);
}

TEST_F(StateMachineTest, ExitState) {
    stateMachine.enterState(TestState::Idle);
    stateMachine.enterState(TestState::Running);
    ASSERT_EQ(exitCount, 1);
}

TEST_F(StateMachineTest, TransitionBetweenStates) {
    stateMachine.enterState(TestState::Idle);
    stateMachine.enterState(TestState::Running);
    stateMachine.enterState(TestState::Stopping);

    ASSERT_EQ(enterCount, 3);
    ASSERT_EQ(exitCount, 2);
    ASSERT_EQ(updateCount, 0);

    stateMachine.update();
    ASSERT_EQ(updateCount, 1);
}

TEST_F(StateMachineTest, ActionModification) {
    stateMachine.enterState(TestState::Idle);
    auto& action = stateMachine.action(TestState::Idle);
    int customActionCount = 0;
    action.onUpdate = [&customActionCount] { customActionCount++; };
    
    stateMachine.update();
    ASSERT_EQ(customActionCount, 1);
}

TEST_F(StateMachineTest, StateExistenceCheck) {
    EXPECT_THROW(stateMachine.action(static_cast<TestState>(999)), std::out_of_range);
}

TEST_F(StateMachineTest, AddExistingState) {
    EXPECT_THROW(stateMachine.addState(TestState::Idle, {}), std::runtime_error);
}
