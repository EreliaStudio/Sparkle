#include <gtest/gtest.h>

#include "network/client.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	struct StressPayload
	{
		std::uint32_t client = 0;
		std::uint32_t sequence = 0;
	};

	[[nodiscard]] bool allSeen(const std::vector<bool> &values)
	{
		return std::all_of(values.begin(), values.end(), [](bool value) {
			return value;
		});
	}
}

TEST(NetworkStressTest, ConcurrentClientsDeliverEveryMessageExactlyOnce)
{
	constexpr std::uint32_t ClientCount = 16;
	constexpr std::uint32_t MessagesPerClient = 100;
	spk::Server server;
	server.start(0);

	std::array<std::unique_ptr<spk::Client>, ClientCount> clients;
	NetworkTestUtils::ThreadFailure failure;
	std::vector<std::jthread> threads;
	for (std::uint32_t client = 0; client < ClientCount; ++client)
	{
		clients[client] = std::make_unique<spk::Client>();
		threads.emplace_back([&, client] {
			failure.run([&, client] {
				clients[client]->connect("127.0.0.1", server.port());
				for (std::uint32_t sequence = 0; sequence < MessagesPerClient; ++sequence)
				{
					spk::Message message(40);
					message << StressPayload{client, sequence};
					clients[client]->send(message);
				}
			});
		});
	}
	for (auto &thread : threads)
	{
		thread.join();
	}
	ASSERT_NO_THROW(failure.rethrow());

	auto received = NetworkTestUtils::collect(
		server.messages(),
		ClientCount * MessagesPerClient,
		10s);
	ASSERT_EQ(received.size(), ClientCount * MessagesPerClient);

	std::array<std::vector<bool>, ClientCount> seen;
	for (auto &client : seen)
	{
		client.resize(MessagesPerClient, false);
	}
	for (spk::ReceivedMessage &entry : received)
	{
		const StressPayload payload = entry.message.get<StressPayload>();
		ASSERT_LT(payload.client, ClientCount);
		ASSERT_LT(payload.sequence, MessagesPerClient);
		EXPECT_FALSE(seen[payload.client][payload.sequence]);
		seen[payload.client][payload.sequence] = true;
	}
	for (const auto &client : seen)
	{
		EXPECT_TRUE(allSeen(client));
	}

	for (auto &client : clients)
	{
		client->disconnect();
	}
	server.stop();
}

TEST(NetworkStressTest, BroadcastBurstPreservesOrderForEveryClient)
{
	constexpr std::uint32_t ClientCount = 8;
	constexpr std::uint32_t MessageCount = 100;
	spk::Server server;
	server.start(0);

	std::array<std::unique_ptr<spk::Client>, ClientCount> clients;
	for (auto &client : clients)
	{
		client = std::make_unique<spk::Client>();
		client->connect("127.0.0.1", server.port());
	}

	for (std::uint32_t sequence = 0; sequence < MessageCount; ++sequence)
	{
		spk::Message message(41);
		message << sequence;
		server.sendToAll(message);
	}

	for (auto &client : clients)
	{
		auto messages = NetworkTestUtils::collect(client->messages(), MessageCount, 10s);
		ASSERT_EQ(messages.size(), MessageCount);
		for (std::uint32_t sequence = 0; sequence < MessageCount; ++sequence)
		{
			EXPECT_EQ(messages[sequence].get<std::uint32_t>(), sequence);
		}
		client->disconnect();
	}
	server.stop();
}

TEST(NetworkStressTest, RepeatedConnectionCyclesRemainUsable)
{
	constexpr std::uint32_t CycleCount = 20;
	spk::Server server;
	spk::Client client;
	server.start(0);

	for (std::uint32_t cycle = 0; cycle < CycleCount; ++cycle)
	{
		client.connect("127.0.0.1", server.port());
		spk::Message message(42);
		message << cycle;
		client.send(message);

		auto received = NetworkTestUtils::collect(server.messages(), 1, 5s);
		ASSERT_EQ(received.size(), 1u);
		EXPECT_EQ(received.front().message.get<std::uint32_t>(), cycle);
		client.disconnect();
	}

	server.stop();
}

TEST(NetworkStressTest, DisconnectingOneClientDuringBroadcastDoesNotBreakOthers)
{
	constexpr std::uint32_t MessageCount = 100;
	spk::Server server;
	spk::Client disconnected;
	spk::Client survivor;
	server.start(0);
	disconnected.connect("127.0.0.1", server.port());
	survivor.connect("127.0.0.1", server.port());

	for (std::uint32_t sequence = 0; sequence < MessageCount / 2; ++sequence)
	{
		spk::Message message(43);
		message << sequence;
		server.sendToAll(message);
	}
	disconnected.disconnect();
	for (std::uint32_t sequence = MessageCount / 2; sequence < MessageCount; ++sequence)
	{
		spk::Message message(43);
		message << sequence;
		server.sendToAll(message);
	}

	auto messages = NetworkTestUtils::collect(survivor.messages(), MessageCount, 10s);
	ASSERT_EQ(messages.size(), MessageCount);
	for (std::uint32_t sequence = 0; sequence < MessageCount; ++sequence)
	{
		EXPECT_EQ(messages[sequence].get<std::uint32_t>(), sequence);
	}

	survivor.disconnect();
	server.stop();
}
