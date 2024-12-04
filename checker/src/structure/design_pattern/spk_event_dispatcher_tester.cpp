#include "structure/design_pattern/spk_event_dispatcher_tester.hpp"

#include <string>

TEST_F(EventDispatcherTest, EmitTriggersSubscribers)
{
	int receivedValue = 0;
	auto contract = spk::EventDispatcher<TestEventType>::subscribe(TestEventType::Number, [&receivedValue](int value) { receivedValue = value; });

	ASSERT_EQ(contract.isValid(), true);
	ASSERT_EQ(receivedValue, 0);

	spk::EventDispatcher<TestEventType>::emit(TestEventType::Number, 42);
	ASSERT_EQ(receivedValue, 42);
}

TEST_F(EventDispatcherTest, ResignStopsReceivingEvents)
{
	int receivedValue = 0;
	auto contract = spk::EventDispatcher<TestEventType>::subscribe(TestEventType::Number, [&receivedValue](int value) { receivedValue = value; });

	spk::EventDispatcher<TestEventType>::emit(TestEventType::Number, 1);
	ASSERT_EQ(receivedValue, 1);

	contract.resign();
	ASSERT_EQ(contract.isValid(), false);

	spk::EventDispatcher<TestEventType>::emit(TestEventType::Number, 2);
	ASSERT_EQ(receivedValue, 1);
}

TEST_F(EventDispatcherTest, MultipleSubscribersAllReceiveEvents)
{
	int first = 0;
	int second = 0;

	auto contract1 = spk::EventDispatcher<TestEventType>::subscribe(TestEventType::Number, [&first](int value) { first = value; });
	auto contract2 = spk::EventDispatcher<TestEventType>::subscribe(TestEventType::Number, [&second](int value) { second = value; });

	spk::EventDispatcher<TestEventType>::emit(TestEventType::Number, 7);

	ASSERT_EQ(first, 7);
	ASSERT_EQ(second, 7);

	contract1.resign();
	contract2.resign();
}

TEST_F(EventDispatcherTest, InvalidatingContractsStopsAllSubscribers)
{
	int callCount = 0;
	auto contract1 = spk::EventDispatcher<TestEventType>::subscribe(
		TestEventType::Number,
		[&callCount](int value)
		{
			(void)value;
			callCount++;
		});
	auto contract2 = spk::EventDispatcher<TestEventType>::subscribe(
		TestEventType::Number,
		[&callCount](int value)
		{
			(void)value;
			callCount++;
		});

	spk::EventDispatcher<TestEventType>::emit(TestEventType::Number, 5);
	ASSERT_EQ(callCount, 2);

	spk::EventDispatcher<TestEventType>::invalidateContracts<int>(TestEventType::Number);
	ASSERT_EQ(contract1.isValid(), false);
	ASSERT_EQ(contract2.isValid(), false);

	spk::EventDispatcher<TestEventType>::emit(TestEventType::Number, 9);
	ASSERT_EQ(callCount, 2);
}

TEST_F(EventDispatcherTest, SubscribeCallableWithNoArguments)
{
	int callCount = 0;
	auto contract = spk::EventDispatcher<TestEventType>::subscribe(TestEventType::Void, [&callCount]() { callCount++; });

	spk::EventDispatcher<TestEventType>::emit(TestEventType::Void);
	ASSERT_EQ(callCount, 1);

	contract.resign();
}

TEST_F(EventDispatcherTest, TypeMismatchThrows)
{
	auto contract = spk::EventDispatcher<TestEventType>::subscribe(TestEventType::Number, [](int value) { (void)value; });

	ASSERT_EQ(contract.isValid(), true);

	EXPECT_THROW(
		spk::EventDispatcher<TestEventType>::subscribe(TestEventType::Number, [](const std::string &text) { (void)text; }), std::runtime_error);
}

TEST_F(EventDispatcherTest, EmitWithoutSubscribersDoesNothing)
{
	spk::EventDispatcher<TestEventType>::emit(TestEventType::Mixed, 1, std::string("data"));

	int received = 0;
	auto contract = spk::EventDispatcher<TestEventType>::subscribe(
		TestEventType::Mixed,
		[&received](int value, const std::string &text)
		{
			received = value;
			ASSERT_EQ(text, "payload");
		});

	spk::EventDispatcher<TestEventType>::emit(TestEventType::Mixed, 3, std::string("payload"));
	ASSERT_EQ(received, 3);

	contract.resign();
}
