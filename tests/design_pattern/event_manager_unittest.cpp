#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "design_pattern/spk_event_manager.hpp"

using namespace spk;

// Define an enum for event types for testing purposes
enum class TestEventType {
    Event1,
    Event2,
    Event3,
};

class EventManagerTest : public ::testing::Test {
protected:
    EventManager<TestEventType> eventManager;
    int callbackInvokeCount = 0;

    virtual void SetUp() override {
        callbackInvokeCount = 0;
    }

    void resetInvokeCount() {
        callbackInvokeCount = 0;
    }
};

TEST_F(EventManagerTest, SubscribeAndNotify) {
    auto contract = eventManager.subscribe(TestEventType::Event1, [this]() { callbackInvokeCount++; });
    eventManager.notify_all(TestEventType::Event1);
    ASSERT_EQ(callbackInvokeCount, 1);
}

TEST_F(EventManagerTest, MultipleSubscriptions) {
    auto contract1 = eventManager.subscribe(TestEventType::Event1, [this]() { callbackInvokeCount++; });
    auto contract2 = eventManager.subscribe(TestEventType::Event1, [this]() { callbackInvokeCount++; });

    eventManager.notify_all(TestEventType::Event1);
    ASSERT_EQ(callbackInvokeCount, 2);
}

TEST_F(EventManagerTest, NoNotificationAfterUnsubscribe) {
    {
        auto contract = eventManager.subscribe(TestEventType::Event1, [this]() { callbackInvokeCount++; });
    } // contract goes out of scope here, which should unsubscribe the callback

    eventManager.notify_all(TestEventType::Event1);
    ASSERT_EQ(callbackInvokeCount, 0);
}

TEST_F(EventManagerTest, SubscribeToMultipleEvents) {
    auto contract1 = eventManager.subscribe(TestEventType::Event1, [this]() { callbackInvokeCount++; });
    auto contract2 = eventManager.subscribe(TestEventType::Event2, [this]() { callbackInvokeCount++; });

    eventManager.notify_all(TestEventType::Event1);
    ASSERT_EQ(callbackInvokeCount, 1);

    resetInvokeCount(); // Resetting the count to test the next event

    eventManager.notify_all(TestEventType::Event2);
    ASSERT_EQ(callbackInvokeCount, 1);
}

TEST_F(EventManagerTest, NoCrossNotification) {
    auto contract1 = eventManager.subscribe(TestEventType::Event1, [this]() { callbackInvokeCount++; });
    auto contract2 = eventManager.subscribe(TestEventType::Event2, [this]() { callbackInvokeCount++; });

    eventManager.notify_all(TestEventType::Event3); // Notifying an event with no subscriptions
    ASSERT_EQ(callbackInvokeCount, 0);
}
