#include "structure/design_pattern/spk_stateful_object_tester.hpp"

TEST_F(StatefulObjectTest, InitialState)
{
    ASSERT_EQ(statefulObject.state(), State::State1) << "Initial state should be State1";
}

TEST_F(StatefulObjectTest, ChangeState)
{
    statefulObject.setState(State::State2);
    ASSERT_EQ(statefulObject.state(), State::State2) << "State should be State2 after setting it to State2";
}

TEST_F(StatefulObjectTest, AddCallbackAndTrigger)
{
    auto contract = statefulObject.addCallback(State::State2, [this]() { incrementCount(); });

    ASSERT_EQ(executionCount, 0) << "Execution count should be 0 before state change";

    statefulObject.setState(State::State2);

    ASSERT_EQ(executionCount, 1) << "Execution count should be 1 after state change triggers callback";
}

TEST_F(StatefulObjectTest, ResignCallback)
{
    auto contract = statefulObject.addCallback(State::State2, [this]() { incrementCount(); });

    contract.resign();

    ASSERT_EQ(executionCount, 0) << "Execution count should be 0 before state change";

    statefulObject.setState(State::State2);

    ASSERT_EQ(executionCount, 0) << "Execution count should still be 0 after state change since callback was resigned";
}

TEST_F(StatefulObjectTest, MultipleCallbacks)
{
    auto contract1 = statefulObject.addCallback(State::State2, [this]() { incrementCount(); });
    auto contract2 = statefulObject.addCallback(State::State2, [this]() { incrementCount(); });

    ASSERT_EQ(executionCount, 0) << "Execution count should be 0 before state change";

    statefulObject.setState(State::State2);

    ASSERT_EQ(executionCount, 2) << "Execution count should be 2 after state change triggers both callbacks";
}

TEST_F(StatefulObjectTest, CallbackOnDifferentState)
{
    auto contract = statefulObject.addCallback(State::State3, [this]() { incrementCount(); });

    ASSERT_EQ(executionCount, 0) << "Execution count should be 0 before state change";

    statefulObject.setState(State::State2);

    ASSERT_EQ(executionCount, 0) << "Execution count should still be 0 after state change to State2";

    statefulObject.setState(State::State3);

    ASSERT_EQ(executionCount, 1) << "Execution count should be 1 after state change to State3 triggers callback";
}

TEST_F(StatefulObjectTest, UnsubscribeCallback)
{
    auto contract = statefulObject.addCallback(State::State2, [this]() { incrementCount(); });

    statefulObject.setState(State::State2);
    ASSERT_EQ(executionCount, 1) << "Execution count should be 1 after state change triggers callback";

    statefulObject.setState(State::State1);
    executionCount = 0;  // Reset execution count for next test

    statefulObject.setState(State::State2);
    ASSERT_EQ(executionCount, 1) << "Execution count should be 1 again after state change triggers callback again";

    contract.resign();
    statefulObject.setState(State::State1);
    executionCount = 0;  // Reset execution count for next test

    statefulObject.setState(State::State2);
    ASSERT_EQ(executionCount, 0) << "Execution count should still be 0 after state change since callback was resigned";
}

TEST_F(StatefulObjectTest, DestructorResignsCallback)
{
    {
        auto contract = statefulObject.addCallback(State::State2, [this]() { incrementCount(); });

        ASSERT_EQ(executionCount, 0) << "Execution count should be 0 before state change";

        statefulObject.setState(State::State2);

        ASSERT_EQ(executionCount, 1) << "Execution count should be 1 after state change triggers callback";
    }

    statefulObject.setState(State::State1);
    executionCount = 0;  // Reset execution count for next test

    statefulObject.setState(State::State2);
    ASSERT_EQ(executionCount, 0) << "Execution count should still be 0 after state change since callback was resigned when contract went out of scope";
}

TEST_F(StatefulObjectTest, DestructorButRelinquisedCallback)
{
    {
        auto contract = statefulObject.addCallback(State::State2, [this]() { incrementCount(); });

        ASSERT_EQ(executionCount, 0) << "Execution count should be 0 before state change";

        statefulObject.setState(State::State2);

        ASSERT_EQ(executionCount, 1) << "Execution count should be 1 after state change triggers callback";

        contract.relinquish();
    }

    statefulObject.setState(State::State1);
    executionCount = 0;  // Reset execution count for next test

    statefulObject.setState(State::State2);
    ASSERT_EQ(executionCount, 1) << "Execution count should be 1 after the transition, with the contract been relinquish toward the provider";
}