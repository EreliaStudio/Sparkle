#include "structure/design_pattern/spk_contract_provider_tester.hpp"
#include <gtest/gtest.h>
#include <string>
#include <utility>

//
// Existing tests (unchanged)
//
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

    provider.trigger();
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

TEST_F(ContractProviderTest, BlockerIgnore_Suppresses_NoFlushOnRelease)
{
    auto contract = provider.subscribe(incrementCountJob);

    int expected = 0;
    ASSERT_EQ(executionCount, expected);

    {
        auto blocker = provider.block(spk::TContractProvider<>::Blocker::Mode::Ignore);
        provider.trigger();
        ASSERT_EQ(blocker.active(), true);
        ASSERT_EQ(executionCount, expected) << "Ignore should drop triggers while active";
    } 

    ASSERT_EQ(executionCount, expected);

    provider.trigger();
    expected = 1;
    ASSERT_EQ(executionCount, expected) << "Next trigger after release should execute";
}

TEST_F(ContractProviderTest, BlockerDelay_FlushesOnceOnRelease)
{
    auto contract = provider.subscribe(incrementCountJob);

    int expected = 0;
    ASSERT_EQ(executionCount, expected);

    {
        auto blocker = provider.block(spk::TContractProvider<>::Blocker::Mode::Delay);
        provider.trigger();
        provider.trigger();
        provider.trigger();
        ASSERT_EQ(executionCount, expected) << "No execution while delayed";
    }

    expected = 1;
    ASSERT_EQ(executionCount, expected) << "Delayed triggers should flush exactly once on release";
}

TEST_F(ContractProviderTest, BlockerDelay_Nested_FlushesAfterLastRelease)
{
    auto contract = provider.subscribe(incrementCountJob);
    int expected = 0;

    auto b1 = provider.block(spk::TContractProvider<>::Blocker::Mode::Delay);
    auto b2 = provider.block(spk::TContractProvider<>::Blocker::Mode::Delay);

    provider.trigger();
    provider.trigger();
    ASSERT_EQ(executionCount, expected);

    b2.release();
    ASSERT_EQ(executionCount, expected) << "Still delayed due to outer block";

    b1.release();
    expected = 1;
    ASSERT_EQ(executionCount, expected) << "Flush once after last delay is released";
}

TEST_F(ContractProviderTest, Blocker_Mix_DelayThenIgnore_ClearsPending)
{
    auto contract = provider.subscribe(incrementCountJob);
    int expected = 0;

    auto delay = provider.block(spk::TContractProvider<>::Blocker::Mode::Delay);
    provider.trigger();

    auto ignore = provider.block(spk::TContractProvider<>::Blocker::Mode::Ignore);
    delay.release();
    ignore.release();

    ASSERT_EQ(executionCount, expected) << "Pending must be cleared by Ignore";

    provider.trigger();
    expected = 1;
    ASSERT_EQ(executionCount, expected) << "Next trigger after all blocks should execute";
}

TEST_F(ContractProviderTest, Contract_MoveSemantics)
{
    auto c1 = provider.subscribe(incrementCountJob);
    ASSERT_TRUE(c1.isValid()) << "Contract should be valid after been created";

    auto c2 = std::move(c1);
    ASSERT_FALSE(c1.isValid()) << "Old object C1 should be invalid after been moved to C2";
    ASSERT_TRUE(c2.isValid()) << "Object C2 should be valid after C1 been moved to C2";

    provider.trigger();
    ASSERT_EQ(executionCount, 1) << "When triggered, the execution count should be 1";

    auto c3 = provider.subscribe(incrementCountJob);
    ASSERT_TRUE(c3.isValid()) << "Contract should be valid after been created";

    c2 = std::move(c3);
    ASSERT_FALSE(c3.isValid()) << "Old object C3 should be invalid after been moved to C2";
    ASSERT_TRUE(c2.isValid()) << "Object C2 should be valid after C3 been moved to C2";

    provider.trigger();
    ASSERT_EQ(executionCount, 2) << "Only 1 contract should run (1st run already happened: now +1)";
}

// --- Double resign throws (defensive)
TEST_F(ContractProviderTest, Contract_DoubleResignThrows)
{
    auto c = provider.subscribe(incrementCountJob);
    ASSERT_TRUE(c.isValid());
    c.resign();
    ASSERT_FALSE(c.isValid());
    EXPECT_ANY_THROW(c.resign());
}

// --- Provider move ctor: subscriptions/blocks/state migrate to destination
TEST(ContractProviderStandalone, Provider_MoveCtor_MigratesSubscriptions)
{
    spk::ContractProvider src;
    int count = 0;
    auto job = [&](){ ++count; };
    auto c = src.subscribe(job);

    // move-construct
    spk::ContractProvider dst(std::move(src));

    // Old provider shouldn't trigger anything
    src.trigger();
    EXPECT_EQ(count, 0);

    // Contract should still be valid and tied to 'dst'
    EXPECT_TRUE(c.isValid());
    dst.trigger();
    EXPECT_EQ(count, 1);

    // Resign via contract should affect destination provider
    c.resign();
    dst.trigger();
    EXPECT_EQ(count, 1);
}

// --- Provider move assignment: invalidate self, adopt other's subscriptions
TEST(ContractProviderStandalone, Provider_MoveAssign_AdoptsAndInvalidates)
{
    spk::ContractProvider a;
    int aCount = 0;
    auto ca = a.subscribe([&](){ ++aCount; });

    spk::ContractProvider b;
    int bCount = 0;
    auto cb = b.subscribe([&](){ ++bCount; });

    // move-assign: 'a' adopts b's state; a's previous subs invalidated
    a = std::move(b);

    // 'a' now triggers b's job; old a job should've been invalidated
    a.trigger();
    EXPECT_EQ(aCount, 0);
    EXPECT_EQ(bCount, 1);

    // Contract 'cb' (created on old 'b') should still be valid and tied to 'a' now
    EXPECT_TRUE(cb.isValid());

    // Contract 'ca' should be invalid because a.invalidateContracts() runs during move-assign
    EXPECT_FALSE(ca.isValid());
}

// --- Blocker move semantics: moving a blocker keeps it active and releases once
TEST(ContractProviderStandalone, Blocker_MoveSemantics)
{
    spk::ContractProvider p;
    int count = 0;
    auto c = p.subscribe([&](){ ++count; });

    auto b1 = p.block(spk::ContractProvider::Blocker::Mode::Delay);
    EXPECT_TRUE(b1.active());

    // Move the blocker
    auto b2 = std::move(b1);
    EXPECT_FALSE(b1.active());
    EXPECT_TRUE(b2.active());

    p.trigger();
    EXPECT_EQ(count, 0);

    // Release the moved-to blocker and expect single flush
    b2.release();
    EXPECT_EQ(count, 1);
}

// --- VoidJob overload works for provider with parameters
TEST(ContractProviderStandalone, VoidJob_Subscribe_WorksWithArgsProvider)
{
    spk::TContractProvider<int> p;
    int count = 0;

    auto c = p.subscribe([&](){ ++count; }); // VoidJob overload
    p.trigger(42);

    EXPECT_EQ(count, 1);
}

// --- Multi-arg provider: Delay coalesces and replays LAST args
TEST(ContractProviderStandalone, Delay_CoalescesLatestArgs)
{
    spk::TContractProvider<int, std::string> p;
    int calls = 0;
    int lastI = -1;
    std::string lastS;

    auto c = p.subscribe([&](int i, const std::string& s){
        ++calls;
        lastI = i;
        lastS = s;
    });

    {
        auto b = p.block(spk::TContractProvider<int, std::string>::Blocker::Mode::Delay);
        p.trigger(1, "first");
        p.trigger(2, "second");
        p.trigger(3, "third");
        EXPECT_EQ(calls, 0);
    } // release -> should flush ONCE with the last args

    EXPECT_EQ(calls, 1);
    EXPECT_EQ(lastI, 3);
    EXPECT_EQ(lastS, "third");
}

// --- Mixed nesting: Ignore outer, Delay inner -> Ignore wins, nothing flushes
TEST(ContractProviderStandalone, MixedBlocks_IgnoreDominatesDelay)
{
    spk::ContractProvider p;
    int count = 0;
    auto c = p.subscribe([&](){ ++count; });

    auto ignore = p.block(spk::ContractProvider::Blocker::Mode::Ignore);
    auto delay  = p.block(spk::ContractProvider::Blocker::Mode::Delay);

    p.trigger();
    EXPECT_EQ(count, 0);

    delay.release();  // still ignored
    EXPECT_EQ(count, 0);

    ignore.release(); // ignoreCount reaches 0; pending should have been cleared by Ignore
    EXPECT_EQ(count, 0);

    p.trigger();
    EXPECT_EQ(count, 1);
}

// --- Unsubscribe after resign is a no-op
TEST_F(ContractProviderTest, UnsubscribeAfterResign_NoThrow_NoEffect)
{
    auto c = provider.subscribe(incrementCountJob);
    c.resign();
    ASSERT_FALSE(c.isValid());

    // Should not crash, and should not change count
    provider.unsubscribe(c);
    provider.trigger();
    ASSERT_EQ(executionCount, 0);
}
