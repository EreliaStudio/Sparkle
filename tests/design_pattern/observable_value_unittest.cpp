#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "design_pattern/spk_observable_value.hpp"

using namespace spk;

class ObservableValueTest : public ::testing::Test {
protected:
    ObservableValue<int> observableInt;
    int notificationCount = 0;

    virtual void SetUp() override {
        observableInt = ObservableValue<int>(0);
        notificationCount = 0;
    }

    void incrementNotificationCount() {
        notificationCount++;
    }
};

TEST_F(ObservableValueTest, NotifyOnChange) {
    auto contract = observableInt.subscribe([this] { incrementNotificationCount(); });
    observableInt = 10;
    ASSERT_EQ(notificationCount, 1);
}

TEST_F(ObservableValueTest, DelayedNotification) {
    auto contract = observableInt.subscribe([this] { incrementNotificationCount(); });
    observableInt.setWithoutNotification(10);
    ASSERT_EQ(notificationCount, 0);
	observableInt.notify_all();
    ASSERT_EQ(notificationCount, 1);
}

TEST_F(ObservableValueTest, ArithmeticOperations) {
    auto contract = observableInt.subscribe([this] { incrementNotificationCount(); });

    observableInt += 5;
    ASSERT_EQ(notificationCount, 1);
    ASSERT_EQ(static_cast<int>(observableInt), 5);

    observableInt -= 3;
    ASSERT_EQ(notificationCount, 2);
    ASSERT_EQ(static_cast<int>(observableInt), 2);

    observableInt *= 4;
    ASSERT_EQ(notificationCount, 3);
    ASSERT_EQ(static_cast<int>(observableInt), 8);

    observableInt /= 2;
    ASSERT_EQ(notificationCount, 4);
    ASSERT_EQ(static_cast<int>(observableInt), 4);
}

TEST_F(ObservableValueTest, SubscriptionManagement)
{
    observableInt = 5;
    auto contract = observableInt.subscribe([this] { incrementNotificationCount(); });
    observableInt = 10;
    ASSERT_EQ(notificationCount, 1);

    contract = nullptr; // Unsubscribe
    observableInt = 15;
    ASSERT_EQ(notificationCount, 1); // Notification count should not increase after unsubscribe
}

TEST_F(ObservableValueTest, MultipleSubscriptions)
{
    observableInt = 5;
    auto contract1 = observableInt.subscribe([this] { incrementNotificationCount(); });
    auto contract2 = observableInt.subscribe([this] { incrementNotificationCount(); });
    observableInt = 10;
    ASSERT_EQ(notificationCount, 2); // Two notifications for the same change
}