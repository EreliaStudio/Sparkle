#include "structure/design_pattern/spk_contract_provider_tester.hpp"

TEST_F(ContractProviderTest, SubscribeJob)
{
    auto contract = provider.subscribe(incrementCountJob);

    int expectedExecutionCount = 0;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    provider.trigger(); // Trigger the job through the provider

    expectedExecutionCount = 1;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 1 after calling trigger() on the provider";
}

TEST_F(ContractProviderTest, ResignJob)
{
    auto contract = provider.subscribe(incrementCountJob);

    ASSERT_EQ(contract.isValid(), true) << "A contract should be valid when created";

    contract.resign();

    int expectedExecutionCount = 0;

    ASSERT_EQ(contract.isValid(), false) << "A contract should be invalid when resigned";
    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    provider.trigger();

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should remain 0 after triggering the provider since the contract was resigned";
}

TEST_F(ContractProviderTest, MultipleJobsExecution)
{
    auto contract1 = provider.subscribe(incrementCountJob);
    auto contract2 = provider.subscribe(incrementCountJob);

    int expectedExecutionCount = 0;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    provider.trigger(); // Trigger both jobs through the provider
    expectedExecutionCount = 2;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 2 after triggering the provider";
}

TEST_F(ContractProviderTest, UnsubscribeJob)
{
    auto contract = provider.subscribe(incrementCountJob);

    provider.unsubscribe(contract);

    int expectedExecutionCount = 0;

    ASSERT_EQ(contract.isValid(), false) << "A contract should be invalid when the provider unsubscribes it";
    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    provider.trigger();

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should remain 0 after triggering the provider since the contract was unsubscribed";
}

TEST_F(ContractProviderTest, ResignRemovesFromProvider)
{
    auto contract = provider.subscribe(incrementCountJob);

    contract.resign();

    int expectedExecutionCount = 0;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    provider.trigger();

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should remain 0 after triggering the provider since the contract was resigned";

    provider.unsubscribe(contract);

    provider.trigger();

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should remain 0 after unsubscribing the contract from the provider";
}

TEST_F(ContractProviderTest, TriggerExecutesAllJobs)
{
    auto contract1 = provider.subscribe(incrementCountJob);
    auto contract2 = provider.subscribe(incrementCountJob);

    int expectedExecutionCount = 0;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    provider.trigger();

    expectedExecutionCount = 2;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 2 after calling trigger";
}

TEST_F(ContractProviderTest, DestructorResignsContract)
{
    int expectedExecutionCount = 0;
    {
        auto contract = provider.subscribe(incrementCountJob);

        ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

        provider.trigger();
        expectedExecutionCount = 1;

        ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 1 after triggering the provider";
    }

    expectedExecutionCount = 1;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 1 after contract goes out of scope and resigns";

    provider.trigger();

    expectedExecutionCount = 1;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should remain 1 after calling trigger() because the contract was resigned upon destruction";
}

TEST_F(ContractProviderTest, RelinquishJob)
{
    auto contract = provider.subscribe(incrementCountJob);

    int expectedExecutionCount = 0;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    contract.relinquish();

    ASSERT_EQ(contract.isValid(), false) << "A contract should be invalid when relinquished";

    provider.trigger();

    expectedExecutionCount = 1;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 1 after triggering the provider even if the contract was relinquished";
}

TEST_F(ContractProviderTest, InvalidateContracts)
{
    auto contract1 = provider.subscribe(incrementCountJob);
    auto contract2 = provider.subscribe(incrementCountJob);

    int expectedExecutionCount = 0;

    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 0 at the start of the test";

    provider.trigger();

    expectedExecutionCount = 2;
    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should be 2 after triggering the provider before all contracts invalidation";

    provider.invalidateContracts();

    provider.trigger();

    expectedExecutionCount = 2;
    ASSERT_EQ(executionCount, expectedExecutionCount) << "Execution count should still be 2 after triggering the provider after all contracts were invalidated";
}
