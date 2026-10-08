#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/client.hpp"
#include "network/remote_node.hpp"
#include "network/server.hpp"

#include <cstdint>
#include <exception>
#include <semaphore>
#include <thread>
#include <type_traits>
#include <vector>

namespace
{
	spk::Message message(spk::Message::Type type, std::uint32_t value = 0)
	{
		spk::Message::Writer writer(type);
		writer.setRequestID(value + 100);
		writer << value;
		return std::move(writer).build();
	}
}

static_assert(std::is_same_v<spk::Client::MessageDispatcher::Callback, std::function<void(const spk::Message &)>>);
static_assert(std::is_same_v<spk::Server::MessageDispatcher::Callback, std::function<void(const spk::ReceivedMessage &)>>);
static_assert(std::is_same_v<spk::RemoteNode::Endpoint::MessageDispatcher::Callback, std::function<void(const spk::RemoteNode::Endpoint::Request &)>>);

TEST(MessageDispatcherTest, MatchesTypesAndPreservesQueueAndSubscriptionOrder)
{
	spk::Client client;
	std::vector<int> seen;
	auto first = client.messageDispatcher().subscribeTo(7, [&](const auto &value) {
		seen.push_back(value.reader().template get<std::uint32_t>() * 10 + 1);
	});
	auto second = client.messageDispatcher().subscribeTo(7, [&](const auto &value) {
		seen.push_back(value.reader().template get<std::uint32_t>() * 10 + 2);
	});
	auto other = client.messageDispatcher().subscribeTo(8, [&](const auto &) {
		seen.push_back(80);
	});
	client.messages().publish(message(7, 1));
	client.messages().publish(message(99));
	client.messages().publish(message(8));
	client.messages().publish(message(7, 2));
	EXPECT_TRUE(seen.empty());
	client.treatMessages();
	EXPECT_EQ(seen, (std::vector<int>{11, 12, 80, 21, 22}));
	client.treatMessages();
	EXPECT_EQ(seen.size(), 5u);
}

TEST(MessageDispatcherTest, ContractDestructionAndResignationUnsubscribe)
{
	spk::Client client;
	int calls = 0;
	{
		auto contract = client.messageDispatcher().subscribeTo(7, [&](const auto &) {
			++calls;
		});
		client.messages().publish(message(7));
		client.treatMessages();
	}
	client.messages().publish(message(7));
	client.treatMessages();
	EXPECT_EQ(calls, 1);
	auto contract = client.messageDispatcher().subscribeTo(7, [&](const auto &) {
		++calls;
	});
	contract.resign();
	client.messages().publish(message(7));
	client.treatMessages();
	EXPECT_EQ(calls, 1);
}

TEST(MessageDispatcherTest, OwnerDestructionInvalidatesContracts)
{
	spk::Client::MessageDispatcher::Contract contract;
	{
		spk::Client client;
		contract = client.messageDispatcher().subscribeTo(7, [](const auto &) {
		});
		EXPECT_TRUE(contract.isValid());
	}
	EXPECT_FALSE(contract.isValid());
	EXPECT_NO_THROW(contract.resign());
}

TEST(MessageDispatcherTest, SubscriptionMutationDuringCallbackIsSafe)
{
	spk::Client client;
	std::vector<int> seen;
	spk::Client::MessageDispatcher::Contract first, second, added;
	first = client.messageDispatcher().subscribeTo(7, [&](const auto &) {
		seen.push_back(1);
		first.resign();
		second.resign();
		added = client.messageDispatcher().subscribeTo(7, [&](const auto &) {
			seen.push_back(3);
		});
	});
	second = client.messageDispatcher().subscribeTo(7, [&](const auto &) {
		seen.push_back(2);
	});
	client.messages().publish(message(7));
	client.messages().publish(message(7));
	client.treatMessages();
	EXPECT_EQ(seen, (std::vector<int>{1, 3}));
}

TEST(MessageDispatcherTest, ProducerThreadDoesNotRunCallbacks)
{
	spk::Client client;
	const auto owner = std::this_thread::get_id();
	std::thread::id callbackThread;
	auto contract = client.messageDispatcher().subscribeTo(7, [&](const auto &) {
		callbackThread = std::this_thread::get_id();
	});
	std::jthread producer([&] {
		client.messages().publish(message(7));
	});
	producer.join();
	EXPECT_EQ(callbackThread, std::thread::id{});
	client.treatMessages();
	EXPECT_EQ(callbackThread, owner);
}

TEST(MessageDispatcherTest, MessagesPublishedDuringCallbackWaitForNextTreatment)
{
	spk::Client client;
	std::vector<std::uint32_t> seen;
	auto contract = client.messageDispatcher().subscribeTo(7, [&](const auto &value) {
		const auto number = value.reader().template get<std::uint32_t>();
		seen.push_back(number);
		if (number == 1)
		{
			client.messages().publish(message(7, 2));
		}
	});
	client.messages().publish(message(7, 1));
	client.treatMessages();
	EXPECT_EQ(seen, (std::vector<std::uint32_t>{1}));
	client.treatMessages();
	EXPECT_EQ(seen, (std::vector<std::uint32_t>{1, 2}));
}

TEST(MessageDispatcherTest, ThrowingCallbackDoesNotReplayOrLoseRemainingMessages)
{
	spk::Client client;
	std::vector<std::uint32_t> seen;
	auto contract = client.messageDispatcher().subscribeTo(7, [&](const auto &value) {
		const auto number = value.reader().template get<std::uint32_t>();
		seen.push_back(number);
		if (number == 1)
		{
			throw spk::Exception("Callback failed");
		}
	});
	client.messages().publish(message(7, 1));
	client.messages().publish(message(7, 2));
	EXPECT_THROW(client.treatMessages(), spk::Exception);
	client.messages().publish(message(7, 3));
	client.treatMessages();
	EXPECT_EQ(seen, (std::vector<std::uint32_t>{1, 2}));
	client.treatMessages();
	EXPECT_EQ(seen, (std::vector<std::uint32_t>{1, 2, 3}));
}

TEST(MessageDispatcherTest, RecursiveTreatmentIsRejectedWithoutCorruptingBatch)
{
	spk::Client client;
	int calls = 0;
	auto contract = client.messageDispatcher().subscribeTo(7, [&](const auto &) {
		++calls;
		EXPECT_THROW(client.treatMessages(), spk::Exception);
	});
	client.messages().publish(message(7));
	client.messages().publish(message(7));
	EXPECT_NO_THROW(client.treatMessages());
	EXPECT_EQ(calls, 2);
}

TEST(MessageDispatcherTest, IndependentInstancesAndEmptyCallbacks)
{
	spk::Client first, second;
	int calls = 0;
	auto contract = first.messageDispatcher().subscribeTo(7, [&](const auto &) {
		++calls;
	});
	second.messages().publish(message(7));
	second.treatMessages();
	EXPECT_EQ(calls, 0);
	EXPECT_THROW((void)first.messageDispatcher().subscribeTo(7, {}), spk::Exception);
}

TEST(MessageDispatcherTest, ServerPreservesEmitterPayloadAndRequestID)
{
	spk::Server server;
	std::vector<spk::ConnectionID> emitters;
	auto contract = server.messageDispatcher().subscribeTo(7, [&](const spk::ReceivedMessage &received) {
		emitters.push_back(received.emitter);
		EXPECT_EQ(received.message.requestID(), 142u);
		EXPECT_EQ(received.message.reader().get<std::uint32_t>(), 42u);
	});
	server.messages().publish({11, message(7, 42)});
	server.messages().publish({22, message(7, 42)});
	server.treatMessages();
	EXPECT_EQ(emitters, (std::vector<spk::ConnectionID>{11, 22}));
}

TEST(MessageDispatcherTest, RemoteEndpointPreservesBothRoutes)
{
	spk::RemoteNode::Endpoint endpoint;
	std::vector<spk::RemoteNode::Endpoint::Request> saved;
	auto contract = endpoint.messageDispatcher().subscribeTo(7, [&](const auto &request) {
		saved.push_back(request);
	});
	endpoint.requests().publish({10, 20, message(7, 42)});
	endpoint.requests().publish({11, 20, message(7, 43)});
	endpoint.treatMessages();
	ASSERT_EQ(saved.size(), 2u);
	EXPECT_EQ(saved[0].proxyConnection, 10u);
	EXPECT_EQ(saved[1].proxyConnection, 11u);
	EXPECT_EQ(saved[0].originConnection, 20u);
	EXPECT_EQ(saved[1].originConnection, 20u);
	EXPECT_EQ(saved[0].message.reader().get<std::uint32_t>(), 42u);
	EXPECT_EQ(saved[1].message.requestID(), 143u);
}

TEST(MessageDispatcherTest, ManualQueueConsumptionRemainsAvailable)
{
	spk::Client client;
	int calls = 0;
	auto contract = client.messageDispatcher().subscribeTo(7, [&](const auto &) {
		++calls;
	});
	client.messages().publish(message(7));
	std::vector<spk::Message> manual;
	(void)client.messages().drain(manual);
	ASSERT_EQ(manual.size(), 1u);
	client.treatMessages();
	EXPECT_EQ(calls, 0);
}

TEST(MessageDispatcherTest, ConcurrentTreatmentIsRejectedAndDispatcherRecovers)
{
	spk::Client client;
	std::binary_semaphore entered(0), release(0);
	std::exception_ptr failure;
	auto contract = client.messageDispatcher().subscribeTo(7, [&](const auto &) {
		entered.release();
		release.acquire();
	});
	client.messages().publish(message(7));
	std::jthread worker([&] {
		try
		{
			client.treatMessages();
		} catch (...)
		{
			failure = std::current_exception();
		}
	});
	entered.acquire();
	EXPECT_THROW(client.treatMessages(), spk::Exception);
	release.release();
	worker.join();
	EXPECT_EQ(failure, nullptr);
	EXPECT_NO_THROW(client.treatMessages());
}

TEST(MessageDispatcherTest, TreatmentSubscriptionsRunBeforeMessagesAndForEmptyQueues)
{
	spk::Client client;
	std::vector<int> seen;
	auto treatment = client.messageDispatcher().subscribeToTreatment([&] {
		seen.push_back(1);
	});
	auto messageContract = client.messageDispatcher().subscribeTo(7, [&](const auto &) {
		seen.push_back(2);
	});
	client.treatMessages();
	client.messages().publish(message(7));
	client.treatMessages();
	EXPECT_EQ(seen, (std::vector<int>{1, 1, 2}));
	treatment.resign();
	client.treatMessages();
	EXPECT_EQ(seen.size(), 3u);
}

TEST(MessageDispatcherTest, ThrowingTreatmentPreservesMessagesAndRejectsReentry)
{
	spk::Client client;
	int received = 0;
	auto subscriber = client.messageDispatcher().subscribeTo(7, [&](const auto &) {
		++received;
	});
	auto treatment = client.messageDispatcher().subscribeToTreatment([&] {
		client.treatMessages();
	});
	client.messages().publish(message(7));
	EXPECT_THROW(client.treatMessages(), spk::Exception);
	EXPECT_EQ(received, 0);
	treatment.resign();
	client.treatMessages();
	EXPECT_EQ(received, 1);
	EXPECT_THROW((void)client.messageDispatcher().subscribeToTreatment({}), spk::Exception);
}
