#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/client.hpp"
#include "network/internal/page_scheduler.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <unordered_map>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	struct StressPayload
	{
		std::uint32_t client = 0;
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
				if (!NetworkTestUtils::waitConnected(*clients[client]))
				{
					throw spk::Exception("Stress client failed to connect.");
				}
				for (std::uint32_t sequence = 0; sequence < MessagesPerClient; ++sequence)
				{
					clients[client]->send(
						makeMessage(
							40,
							StressPayload{client, sequence}));
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
	for (const spk::ReceivedMessage &entry : received)
	{
		const StressPayload payload =
			entry.message.reader().get<StressPayload>();
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
	std::atomic_uint32_t connected = 0;
	auto contract = server.subscribeToConnectionStatusChange(
		[&](spk::Connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status == spk::Connection::Status::Connected)
			{
				++connected;
			}
		});
	server.start(0);

	std::array<std::unique_ptr<spk::Client>, ClientCount> clients;
	for (auto &client : clients)
	{
		client = std::make_unique<spk::Client>();
		client->connect("127.0.0.1", server.port());
	}

	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		return connected.load() == ClientCount;
	},
											5s));

	for (std::uint32_t sequence = 0; sequence < MessageCount; ++sequence)
	{
		server.sendToAll(makeMessage(41, sequence));
	}

	for (auto &client : clients)
	{
		auto messages =
			NetworkTestUtils::collect(
				client->messages(),
				MessageCount,
				10s);
		ASSERT_EQ(messages.size(), MessageCount);
		for (std::uint32_t sequence = 0; sequence < MessageCount; ++sequence)
		{
			EXPECT_EQ(
				messages[sequence].reader().get<std::uint32_t>(),
				sequence);
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
		EXPECT_TRUE(NetworkTestUtils::waitConnected(client));
		client.send(makeMessage(42, cycle));

		auto received = NetworkTestUtils::collect(server.messages(), 1, 5s);
		ASSERT_EQ(received.size(), 1u);
		EXPECT_EQ(
			received.front().message.reader().get<std::uint32_t>(),
			cycle);
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
	std::atomic_uint32_t connected = 0;
	auto contract = server.subscribeToConnectionStatusChange(
		[&](spk::Connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status == spk::Connection::Status::Connected)
			{
				++connected;
			}
		});
	server.start(0);
	disconnected.connect("127.0.0.1", server.port());
	EXPECT_TRUE(NetworkTestUtils::waitConnected(disconnected));
	survivor.connect("127.0.0.1", server.port());
	EXPECT_TRUE(NetworkTestUtils::waitConnected(survivor));

	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		return connected.load() == 2;
	},
											5s));

	for (std::uint32_t sequence = 0; sequence < MessageCount / 2; ++sequence)
	{
		server.sendToAll(makeMessage(43, sequence), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::SkipRecipient);
	}
	disconnected.disconnect();
	for (std::uint32_t sequence = MessageCount / 2; sequence < MessageCount; ++sequence)
	{
		server.sendToAll(makeMessage(43, sequence), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::SkipRecipient);
	}

	auto messages =
		NetworkTestUtils::collect(
			survivor.messages(),
			MessageCount,
			10s);
	ASSERT_EQ(messages.size(), MessageCount);
	for (std::uint32_t sequence = 0; sequence < MessageCount; ++sequence)
	{
		EXPECT_EQ(
			messages[sequence].reader().get<std::uint32_t>(),
			sequence);
	}

	survivor.disconnect();
	server.stop();
}

TEST(NetworkStressTest, SeededRandomPayloadsPreserveEveryByte)
{
	constexpr std::uint32_t Seed = 0x51AC2026;
	constexpr std::size_t MessageCount = 160;
	std::mt19937 random(Seed);
	std::uniform_int_distribution<std::size_t> size(1, 32768);
	std::vector<std::vector<std::byte>> expected(MessageCount);
	std::size_t expectedBytes = 0;
	spk::Server server;
	spk::Client client;
	server.setMaxPageBytes(2048);
	client.setMaxPageBytes(2048);
	server.start(0);
	client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));
	for (std::size_t index = 0; index < MessageCount; ++index)
	{
		spk::Message::Writer writer(410);
		writer.setRequestID(index + 1);
		writer.resize(size(random));
		for (std::byte &value : writer.data())
		{
			value = static_cast<std::byte>(random() & 255u);
		}
		expected[index].assign(writer.data().begin(), writer.data().end());
		expectedBytes += expected[index].size();
		(void)client.send(std::move(writer).build());
	}
	const auto received = NetworkTestUtils::collect(server.messages(), MessageCount, 30s);
	ASSERT_EQ(received.size(), MessageCount);
	std::vector<bool> seen(MessageCount, false);
	std::size_t actualBytes = 0;
	for (const spk::ReceivedMessage &entry : received)
	{
		ASSERT_EQ(entry.message.type(), 410u);
		const auto request = entry.message.requestID();
		ASSERT_GT(request, 0u);
		ASSERT_LE(request, MessageCount);
		const auto index = static_cast<std::size_t>(request - 1);
		EXPECT_FALSE(seen[index]);
		seen[index] = true;
		actualBytes += entry.message.size();
		EXPECT_EQ(std::vector<std::byte>(entry.message.data().begin(), entry.message.data().end()), expected[index]);
	}
	EXPECT_TRUE(allSeen(seen));
	EXPECT_EQ(actualBytes, expectedBytes);
	std::cout << "seed=" << Seed << " messages=" << received.size() << " verified_bytes=" << actualBytes << '\n';
	client.disconnect();
	server.stop();
}

TEST(NetworkStressTest, SeededSchedulerAccountsForRandomPagesAndPriorities)
{
	constexpr std::uint32_t Seed = 0x1234ABC;
	constexpr std::size_t MessageCount = 256;
	std::mt19937 random(Seed);
	std::uniform_int_distribution<std::size_t> size(1, 16384);
	std::uniform_int_distribution<std::size_t> pageSize(257, 4096);
	spk::NetworkInternal::PageScheduler scheduler;
	std::unordered_map<std::uint64_t, std::vector<std::byte>> expected;
	std::unordered_map<std::uint64_t, std::vector<bool>> covered;
	std::size_t expectedBytes = 0;
	for (std::size_t index = 0; index < MessageCount; ++index)
	{
		spk::Message::Writer writer(411);
		writer.resize(size(random));
		for (std::byte &value : writer.data())
		{
			value = static_cast<std::byte>(random() & 255u);
		}
		const auto bytes = std::vector<std::byte>(writer.data().begin(), writer.data().end());
		const auto priority = static_cast<spk::Message::Priority>(index % 3);
		const auto id = scheduler.enqueue(std::move(writer).build(), priority);
		covered.emplace(id, std::vector<bool>(bytes.size(), false));
		expectedBytes += bytes.size();
		expected.emplace(id, bytes);
	}
	EXPECT_EQ(scheduler.pendingTransfers(), MessageCount);
	std::size_t actualBytes = 0;
	std::size_t actualPages = 0;
	while (const auto page = scheduler.nextPage(pageSize(random)))
	{
		ASSERT_LT(actualPages++, 20000u);
		const auto it = expected.find(page->transferID);
		ASSERT_NE(it, expected.end());
		ASSERT_EQ(page->totalSize, it->second.size());
		ASSERT_LE(page->offset, it->second.size());
		ASSERT_LE(page->length, it->second.size() - page->offset);
		const auto bytes = page->data();
		EXPECT_TRUE(std::equal(bytes.begin(), bytes.end(), it->second.begin() + page->offset));
		auto &marks = covered.at(page->transferID);
		for (std::size_t index = 0; index < page->length; ++index)
		{
			EXPECT_FALSE(marks[page->offset + index]);
			marks[page->offset + index] = true;
		}
		actualBytes += page->length;
	}
	EXPECT_EQ(scheduler.pendingTransfers(), 0u);
	EXPECT_EQ(scheduler.pendingBytes(), 0u);
	EXPECT_EQ(actualBytes, expectedBytes);
	for (const auto &[id, marks] : covered)
	{
		EXPECT_TRUE(allSeen(marks)) << id;
	}
	std::cout << "seed=" << Seed << " transfers=" << MessageCount << " pages=" << actualPages << " verified_bytes=" << actualBytes << '\n';
}
