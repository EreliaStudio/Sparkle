#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "design_pattern/spk_notifier.hpp"

using namespace spk;

class NotifierTest : public ::testing::Test {
protected:
    Notifier notifier;
    int notificationCount = 0;

    virtual void SetUp() override {
        notificationCount = 0;
    }

    void incrementCount() {
        notificationCount++;
    }
};

TEST_F(NotifierTest, NotifyAll) {
    auto contract = notifier.subscribe([this] { incrementCount(); });
    notifier.notify_all();
    ASSERT_EQ(notificationCount, 1);
}

TEST_F(NotifierTest, MultipleSubscriptions) {
    auto contract1 = notifier.subscribe([this] { incrementCount(); });
    auto contract2 = notifier.subscribe([this] { incrementCount(); });
    notifier.notify_all();
    ASSERT_EQ(notificationCount, 2);
}

TEST_F(NotifierTest, SubscriptionCancellation) {
    {
        auto contract = notifier.subscribe([this] { incrementCount(); });
    } // The contract is out of scope, so the callback should not be called
    notifier.notify_all();
    ASSERT_EQ(notificationCount, 0);
}

TEST_F(NotifierTest, PauseAndResumeSubscription) {
    auto contract = notifier.subscribe([this] { incrementCount(); });
    contract->pause();
    
    notifier.notify_all();
    ASSERT_EQ(notificationCount, 0); // Notification should not increase count due to pause

    contract->resume();
    
    notifier.notify_all();

    ASSERT_EQ(notificationCount, 1); // Now the notification should be effective
}

TEST_F(NotifierTest, CheckIsPausedAndIsCanceled) {
    auto contract = notifier.subscribe([this] { incrementCount(); });
    ASSERT_FALSE(contract->isPaused());
    ASSERT_FALSE(contract->isCanceled());

    contract->pause();
    ASSERT_TRUE(contract->isPaused());
    ASSERT_FALSE(contract->isCanceled());

    contract->resume();
    ASSERT_FALSE(contract->isPaused());

    contract->cancel();
    ASSERT_TRUE(contract->isCanceled());
}
