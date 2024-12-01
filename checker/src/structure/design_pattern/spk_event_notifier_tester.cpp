#include "structure/design_pattern/spk_event_notifier_tester.hpp"

TEST_F(EventNotifierTest, SubscribeJob)
{
    auto contract = notifier->subscribe("TestEvent", incrementCountJob);

    int expectedExecutionCount = 0;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    notifier->notifyEvent("TestEvent");

    expectedExecutionCount = 1;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 1 after notifying the event";
}

TEST_F(EventNotifierTest, ResignJob)
{
    auto contract = notifier->subscribe("TestEvent", incrementCountJob);

    ASSERT_EQ(contract.isValid(), true) << "A contract should be valid when created";

    contract.resign();

    int expectedExecutionCount = 0;

    ASSERT_EQ(contract.isValid(), false) << "A contract should be invalid when resigned";
    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    notifier->notifyEvent("TestEvent");

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 after notifying an event without contract";
}

TEST_F(EventNotifierTest, MultipleJobsExecution)
{
    auto contract1 = notifier->subscribe("TestEvent", incrementCountJob);
    auto contract2 = notifier->subscribe("TestEvent", incrementCountJob);

    int expectedExecutionCount = 0;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    notifier->notifyEvent("TestEvent");
    expectedExecutionCount = 2;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 2 after notifying the event";
}

TEST_F(EventNotifierTest, UnsubscribeJob)
{
    auto contract = notifier->subscribe("TestEvent", incrementCountJob);

	contract.resign();

    int expectedExecutionCount = 0;

    ASSERT_EQ(contract.isValid(), false) << "A contract should be invalid when the provider unsubscribe it";
    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    notifier->notifyEvent("TestEvent");
}

TEST_F(EventNotifierTest, InvalidatedJob)
{
    auto contract = notifier->subscribe("TestEvent", incrementCountJob);

    notifier->invalidateContracts("TestEvent");

    int expectedExecutionCount = 0;

    ASSERT_EQ(contract.isValid(), false) << "A contract should be invalid when the provider unsubscribe it";
    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    notifier->notifyEvent("TestEvent");

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 after notifying invalidated contracts";
}

TEST_F(EventNotifierTest, ResignRemovesFromProvider)
{
    auto contract = notifier->subscribe("TestEvent", incrementCountJob);

    contract.resign();

    int expectedExecutionCount = 0;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    notifier->notifyEvent("TestEvent");

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should still be 0 after trying to notify a resigned event";
}

TEST_F(EventNotifierTest, NotifyExecutesAllJobs)
{
    auto contract1 = notifier->subscribe("TestEvent", incrementCountJob);
    auto contract2 = notifier->subscribe("TestEvent", incrementCountJob);

    int expectedExecutionCount = 0;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    notifier->notifyEvent("TestEvent");

    expectedExecutionCount = 2;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 2 after notifying the event";
}

TEST_F(EventNotifierTest, RelinquishJob)
{
    auto contract = notifier->subscribe("TestEvent", incrementCountJob);

    int expectedExecutionCount = 0;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    contract.relinquish();

    notifier->notifyEvent("TestEvent");

    expectedExecutionCount = 1;
    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 1 after calling notifyEvent() on the notifier";
}