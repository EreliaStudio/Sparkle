#include "structure/design_pattern/spk_observable_value_tester.hpp"

TEST_F(ObservableValueTest, SubscribeJob)
{
    auto contract = value.subscribe(incrementCountJob);

    int expectedExecutionCount = 0;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    value.set(1);

    expectedExecutionCount = 1;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 1 after setting a new value";
}

TEST_F(ObservableValueTest, ResignJob)
{
    auto contract = value.subscribe(incrementCountJob);

    ASSERT_EQ(contract.isValid(), true) << "A contract should be valid when created";

    contract.resign();

    int expectedExecutionCount = 0;

    ASSERT_EQ(contract.isValid(), false) << "A contract should be invalid when resigned";
    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    value.set(1);

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 after editing the value with a resigned contract";
}

TEST_F(ObservableValueTest, MultipleJobsExecution)
{
    auto contract1 = value.subscribe(incrementCountJob);
    auto contract2 = value.subscribe(incrementCountJob);

    int expectedExecutionCount = 0;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    value.set(1);
    expectedExecutionCount = 2;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 2 after setting a new value";
}

TEST_F(ObservableValueTest, DestructorResignsContract)
{
    int expectedExecutionCount = 0;
    {
        auto contract = value.subscribe(incrementCountJob);

        ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

        value.set(1);
        expectedExecutionCount = 1;

        ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 1 after setting a new value";
    }

    expectedExecutionCount = 1;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 1 after contract goes out of scope and resigns";

    value.set(2);

    expectedExecutionCount = 1;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should remain 1 after setting a new value because the contract was resigned upon destruction";
}