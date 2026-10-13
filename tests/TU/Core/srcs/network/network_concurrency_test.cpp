#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/client.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	struct SequencePayload
	{
		std::uint32_t producer = 0;
		std::uint32_t sequence = 0;
	};

	template <typename TValue>
	[[nodiscard]] spk::Message makeMessage(
		spk::Message::Type type,
		const TValue &value)
	{
		spk::Message::Writer writer(type);
		writer << value;
		return std::move(writer).build();
	}

	[[nodiscard]] spk::ConnectionID connectAndIdentify(
		spk::Server &server,
		spk::Client &client)
	{
		client.connect("127.0.0.1", server.port());
		EXPECT_TRUE(NetworkTestUtils::waitConnected(client));
		client.send(makeMessage(1, std::uint32_t{0xCAFE}));

		auto received = NetworkTestUtils::collect(server.messages(), 1);
		if (received.empty())
		{
			return spk::InvalidConnectionID;
		}
		return received.front().emitter;
	}
}

TEST(NetworkConcurrencyTest, ClientDestructorDoesNotAccessOwnerAfterExplicitDisconnect)
{
	for (std::size_t attempt = 0; attempt < 64; ++attempt)
	{
		auto client = std::make_unique<spk::Client>();
		client->disconnect();
		EXPECT_FALSE(client->isConnected());
	}
}

TEST(NetworkConcurrencyTest, ClientDestructorDisconnectsActiveWorkersBeforeDestroyingOwner)
{
	spk::Server server;
	server.start(0);
	for (std::uint32_t attempt = 0; attempt < 8; ++attempt)
	{
		auto client = std::make_unique<spk::Client>();
		client->connect("127.0.0.1", server.port());
		ASSERT_TRUE(NetworkTestUtils::waitConnected(*client));
		(void)client->send(makeMessage(93, attempt));
		auto received = NetworkTestUtils::collect(server.messages(), 1, 5s);
		ASSERT_EQ(received.size(), 1u);
		EXPECT_EQ(received.front().message.reader().get<std::uint32_t>(), attempt);
	}
	server.stop();
}

TEST(NetworkConcurrencyTest, ServerAndClientCanBeDrivenFromIndependentThreads)
{
	spk::Server server;
	spk::Client client;
	NetworkTestUtils::ThreadFailure failure;
	std::atomic_uint16_t port = 0;

	std::jthread serverThread([&] {
		failure.run([&] {
			server.start(0);
			port = server.port();
		});
	});
	serverThread.join();
	ASSERT_NE(port.load(), 0u);

	std::jthread clientThread([&] {
		failure.run([&] {
			client.connect("127.0.0.1", port.load());
			EXPECT_TRUE(NetworkTestUtils::waitConnected(client));
			client.send(makeMessage(21, std::uint32_t{42}));
		});
	});
	clientThread.join();
	ASSERT_NO_THROW(failure.rethrow());

	auto received = NetworkTestUtils::collect(server.messages(), 1);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().message.type(), 21u);
	EXPECT_EQ(received.front().message.reader().get<std::uint32_t>(), 42u);

	client.disconnect();
	server.stop();
}

TEST(NetworkConcurrencyTest, LargePayloadCrossesReceptionBufferBoundaries)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	client.connect("127.0.0.1", server.port());
	EXPECT_TRUE(NetworkTestUtils::waitConnected(client));

	std::vector<std::byte> payload(512u * 1024u);
	for (std::size_t index = 0; index < payload.size(); ++index)
	{
		payload[index] = static_cast<std::byte>(index % 251u);
	}

	spk::Message::Writer writer(22);
	writer.append(payload.data(), payload.size());
	client.send(std::move(writer).build());

	auto received = NetworkTestUtils::collect(server.messages(), 1, 5s);
	ASSERT_EQ(received.size(), 1u);
	ASSERT_EQ(received.front().message.size(), payload.size());
	EXPECT_EQ(
		std::memcmp(
			received.front().message.data().data(),
			payload.data(),
			payload.size()),
		0);

	client.disconnect();
	server.stop();
}

TEST(NetworkConcurrencyTest, ClientBurstPreservesMessageOrder)
{
	constexpr std::uint32_t MessageCount = 250;
	spk::Server server;
	spk::Client client;
	server.start(0);
	client.connect("127.0.0.1", server.port());
	EXPECT_TRUE(NetworkTestUtils::waitConnected(client));

	for (std::uint32_t sequence = 0; sequence < MessageCount; ++sequence)
	{
		client.send(makeMessage(23, sequence));
	}

	auto received = NetworkTestUtils::collect(server.messages(), MessageCount, 5s);
	ASSERT_EQ(received.size(), MessageCount);
	for (std::uint32_t sequence = 0; sequence < MessageCount; ++sequence)
	{
		EXPECT_EQ(
			received[sequence].message.reader().get<std::uint32_t>(),
			sequence);
	}

	client.disconnect();
	server.stop();
}

TEST(NetworkConcurrencyTest, ServerQueuedBurstPreservesMessageOrder)
{
	constexpr std::uint32_t MessageCount = 250;
	spk::Server server;
	spk::Client client;
	server.start(0);
	const spk::ConnectionID connection = connectAndIdentify(server, client);
	ASSERT_NE(connection, spk::InvalidConnectionID);

	for (std::uint32_t sequence = 0; sequence < MessageCount; ++sequence)
	{
		server.sendTo(server.connection(connection), makeMessage(24, sequence));
	}

	auto received = NetworkTestUtils::collect(client.messages(), MessageCount, 5s);
	ASSERT_EQ(received.size(), MessageCount);
	for (std::uint32_t sequence = 0; sequence < MessageCount; ++sequence)
	{
		EXPECT_EQ(
			received[sequence].reader().get<std::uint32_t>(),
			sequence);
	}

	client.disconnect();
	server.stop();
}

TEST(NetworkConcurrencyTest, TargetedSendDoesNotReachOtherClients)
{
	spk::Server server;
	spk::Client first;
	spk::Client second;
	server.start(0);

	const spk::ConnectionID firstID = connectAndIdentify(server, first);
	const spk::ConnectionID secondID = connectAndIdentify(server, second);
	ASSERT_NE(firstID, spk::InvalidConnectionID);
	ASSERT_NE(secondID, spk::InvalidConnectionID);
	ASSERT_NE(firstID, secondID);

	server.sendTo(server.connection(firstID), makeMessage(25, std::uint32_t{91}));

	auto firstMessages = NetworkTestUtils::collect(first.messages(), 1);
	ASSERT_EQ(firstMessages.size(), 1u);
	EXPECT_EQ(
		firstMessages.front().reader().get<std::uint32_t>(),
		91u);

	std::vector<spk::Message> secondMessages;
	std::this_thread::sleep_for(50ms);
	second.messages().drain(secondMessages);
	EXPECT_TRUE(secondMessages.empty());

	first.disconnect();
	second.disconnect();
	server.stop();
}

TEST(NetworkConcurrencyTest, BroadcastReachesEveryConnectedClient)
{
	spk::Server server;
	spk::Client first;
	spk::Client second;
	server.start(0);

	ASSERT_NE(connectAndIdentify(server, first), spk::InvalidConnectionID);
	ASSERT_NE(connectAndIdentify(server, second), spk::InvalidConnectionID);

	server.sendToAll(makeMessage(26, std::string("broadcast")));

	auto firstMessages = NetworkTestUtils::collect(first.messages(), 1);
	auto secondMessages = NetworkTestUtils::collect(second.messages(), 1);
	ASSERT_EQ(firstMessages.size(), 1u);
	ASSERT_EQ(secondMessages.size(), 1u);

	auto firstReader = firstMessages.front().reader();
	auto secondReader = secondMessages.front().reader();
	std::string firstText;
	std::string secondText;
	firstReader >> firstText;
	secondReader >> secondText;
	EXPECT_EQ(firstText, "broadcast");
	EXPECT_EQ(secondText, "broadcast");

	first.disconnect();
	second.disconnect();
	server.stop();
}

TEST(NetworkConcurrencyTest, ConcurrentServerProducersAreSerializedSafely)
{
	constexpr std::uint32_t ProducerCount = 4;
	constexpr std::uint32_t MessageCount = 100;

	spk::Server server;
	spk::Client client;
	server.start(0);
	const spk::ConnectionID connection = connectAndIdentify(server, client);
	ASSERT_NE(connection, spk::InvalidConnectionID);

	NetworkTestUtils::ThreadFailure failure;
	std::vector<std::jthread> producers;
	for (std::uint32_t producer = 0; producer < ProducerCount; ++producer)
	{
		producers.emplace_back([&, producer] {
			failure.run([&] {
				for (std::uint32_t sequence = 0; sequence < MessageCount; ++sequence)
				{
					server.sendTo(
						server.connection(connection),
						makeMessage(
							27,
							SequencePayload{producer, sequence}));
				}
			});
		});
	}
	for (auto &producer : producers)
	{
		producer.join();
	}
	ASSERT_NO_THROW(failure.rethrow());

	auto received = NetworkTestUtils::collect(
		client.messages(),
		ProducerCount * MessageCount,
		5s);
	ASSERT_EQ(received.size(), ProducerCount * MessageCount);

	std::array<std::uint32_t, ProducerCount> next{};
	for (const spk::Message &message : received)
	{
		const SequencePayload payload =
			message.reader().get<SequencePayload>();
		ASSERT_LT(payload.producer, ProducerCount);
		EXPECT_EQ(payload.sequence, next[payload.producer]);
		++next[payload.producer];
	}
	for (const std::uint32_t count : next)
	{
		EXPECT_EQ(count, MessageCount);
	}

	client.disconnect();
	server.stop();
}

TEST(NetworkConcurrencyTest, DisconnectCallbacksIdentifyEachConnection)
{
	spk::Server server;
	spk::Client first;
	spk::Client second;
	std::mutex disconnectedMutex;
	std::vector<spk::ConnectionID> disconnected;

	auto contract = server.subscribeToConnectionStatusChange(
		[&](spk::Connection connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status != spk::Connection::Status::Disconnected)
			{
				return;
			}
			const std::scoped_lock lock(disconnectedMutex);
			disconnected.push_back(connection.id());
		});

	server.start(0);
	const spk::ConnectionID firstID = connectAndIdentify(server, first);
	const spk::ConnectionID secondID = connectAndIdentify(server, second);

	first.disconnect();
	second.disconnect();
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		const std::scoped_lock lock(disconnectedMutex);
		return disconnected.size() == 2;
	}));

	{
		const std::scoped_lock lock(disconnectedMutex);
		EXPECT_NE(
			std::find(disconnected.begin(), disconnected.end(), firstID),
			disconnected.end());
		EXPECT_NE(
			std::find(disconnected.begin(), disconnected.end(), secondID),
			disconnected.end());
	}

	server.stop();
}

TEST(NetworkConcurrencyTest, ServerAndClientCanRestartAndExchangeAgain)
{
	spk::Server server;
	spk::Client client;

	for (std::uint32_t iteration = 0; iteration < 3; ++iteration)
	{
		server.start(0);
		client.connect("127.0.0.1", server.port());
		EXPECT_TRUE(NetworkTestUtils::waitConnected(client));

		client.send(makeMessage(28, iteration));

		auto received = NetworkTestUtils::collect(server.messages(), 1);
		ASSERT_EQ(received.size(), 1u);
		EXPECT_EQ(
			received.front().message.reader().get<std::uint32_t>(),
			iteration);

		client.disconnect();
		server.stop();
	}
}

TEST(NetworkConcurrencyTest, ClientLifecycleCallbacksFireExactlyOnce)
{
	spk::Server server;
	spk::Client client;
	std::atomic_uint32_t connected = 0;
	std::atomic_uint32_t disconnected = 0;
	auto statusContract = client.subscribeToConnectionStatusChange(
		[&](spk::Connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status == spk::Connection::Status::Connected)
			{
				++connected;
			}
			if (status == spk::Connection::Status::Disconnected)
			{
				++disconnected;
			}
		});

	server.start(0);
	client.connect("127.0.0.1", server.port());
	EXPECT_TRUE(NetworkTestUtils::waitConnected(client));
	client.disconnect();
	client.disconnect();
	server.stop();

	EXPECT_EQ(connected.load(), 1u);
	EXPECT_EQ(disconnected.load(), 1u);
}

TEST(NetworkConcurrencyTest, ServerStopDisconnectsConnectedClient)
{
	spk::Server server;
	spk::Client client;
	std::atomic_uint32_t disconnected = 0;
	auto contract = client.subscribeToConnectionStatusChange(
		[&](spk::Connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status == spk::Connection::Status::Disconnected)
			{
				++disconnected;
			}
		});

	server.start(0);
	client.connect("127.0.0.1", server.port());
	EXPECT_TRUE(NetworkTestUtils::waitConnected(client));
	server.stop();

	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		return disconnected.load() == 1;
	}));
	EXPECT_FALSE(client.isConnected());

	client.disconnect();
	EXPECT_EQ(disconnected.load(), 1u);
}

TEST(NetworkConcurrencyTest, ServerCanStopFromConnectionCallback)
{
	spk::Server server;
	spk::Client client;
	std::atomic_bool stopRequested = false;
	auto contract = server.subscribeToConnectionStatusChange(
		[&](spk::Connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status == spk::Connection::Status::Connected)
			{
				stopRequested = true;
				server.stop();
			}
		});

	server.start(0);
	client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		return stopRequested.load();
	}));
	EXPECT_TRUE(NetworkTestUtils::waitUntil([&] {
		return !server.isRunning() && !client.isConnected();
	}));
	server.stop();
}

TEST(NetworkConcurrencyTest, ManyClientsConnectAndExchangeConcurrently)
{
	constexpr std::uint32_t ClientCount = 16;
	spk::Server server;
	server.start(0);

	std::array<std::unique_ptr<spk::Client>, ClientCount> clients;
	std::array<std::vector<spk::Message>, ClientCount> responses;
	NetworkTestUtils::ThreadFailure failure;
	std::vector<std::jthread> threads;
	for (std::uint32_t index = 0; index < ClientCount; ++index)
	{
		clients[index] = std::make_unique<spk::Client>();
		threads.emplace_back([&, index] {
			failure.run([&, index] {
				clients[index]->connect("127.0.0.1", server.port());
				EXPECT_TRUE(NetworkTestUtils::waitConnected(*clients[index]));
				clients[index]->send(makeMessage(29, index));
				responses[index] =
					NetworkTestUtils::collect(
						clients[index]->messages(),
						1,
						5s);
			});
		});
	}

	auto requests = NetworkTestUtils::collect(server.messages(), ClientCount, 5s);
	if (requests.size() == ClientCount)
	{
		for (spk::ReceivedMessage &request : requests)
		{
			const std::uint32_t index =
				request.message.reader().get<std::uint32_t>();
			if (index >= ClientCount)
			{
				continue;
			}
			server.sendTo(server.connection(request.emitter), makeMessage(30, index));
		}
	}

	for (auto &thread : threads)
	{
		thread.join();
	}
	for (auto &client : clients)
	{
		client->disconnect();
	}
	server.stop();

	ASSERT_NO_THROW(failure.rethrow());
	ASSERT_EQ(requests.size(), ClientCount);
	for (std::uint32_t index = 0; index < ClientCount; ++index)
	{
		ASSERT_EQ(responses[index].size(), 1u);
		EXPECT_EQ(
			responses[index].front().reader().get<std::uint32_t>(),
			index);
	}
}

TEST(NetworkConcurrencyTest, EmptyPayloadMessageRoundTrips)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	client.connect("127.0.0.1", server.port());
	EXPECT_TRUE(NetworkTestUtils::waitConnected(client));

	client.send(spk::Message::Writer(31).build());
	auto received = NetworkTestUtils::collect(server.messages(), 1);

	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().message.type(), 31u);
	EXPECT_TRUE(received.front().message.empty());

	client.disconnect();
	server.stop();
}

TEST(NetworkConcurrencyTest, SendingToUnknownConnectionDoesNotAffectConnectedClient)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	const spk::ConnectionID connection = connectAndIdentify(server, client);
	ASSERT_NE(connection, spk::InvalidConnectionID);

	EXPECT_THROW(server.connection(connection + 100000u), spk::Exception);
	std::this_thread::sleep_for(50ms);

	std::vector<spk::Message> received;
	client.messages().drain(received);
	EXPECT_TRUE(received.empty());

	client.disconnect();
	server.stop();
}

TEST(NetworkConcurrencyTest, ClientCanDisconnectFromConnectedStatusCallback)
{
	spk::Server server;
	spk::Client client;
	std::atomic_bool completed = false;
	auto contract = client.subscribeToConnectionStatusChange(
		[&](spk::Connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status == spk::Connection::Status::Connected)
			{
				client.disconnect();
				completed = true;
			}
		});
	server.start(0);
	const auto connection = client.connect("127.0.0.1", server.port());
	const auto callbackCompleted = [&] {
		return completed.load();
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil(callbackCompleted, 5s));
	EXPECT_EQ(connection.status(), spk::Connection::Status::Disconnected);
	EXPECT_FALSE(client.isConnected());
	client.disconnect();
	server.stop();
}
