#include "network/client.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <utility>

using namespace std::chrono_literals;

namespace
{
	[[nodiscard]] spk::Message makeMessage(std::uint32_t type, std::size_t bytes)
	{
		spk::Message::Writer writer(type);
		writer.resize(bytes);
		return std::move(writer).build();
	}

	[[nodiscard]] spk::Connection identify(spk::Server &server, spk::Client &client)
	{
		client.connect("127.0.0.1", server.port());
		EXPECT_TRUE(NetworkTestUtils::waitConnected(client));
		(void)client.send(makeMessage(150, 1));
		auto received = NetworkTestUtils::collect(server.messages(), 1, 5s);
		if (received.empty())
		{
			return {};
		}
		return server.connection(received.front().emitter);
	}

	[[nodiscard]] bool finished(const spk::Transfer &transfer, spk::Transfer::Status status)
	{
		return NetworkTestUtils::waitUntil([&] {
			return transfer.status() == status;
		}, 5s);
	}
}

TEST(ServerTransferIntegration, TargetedSendRetainsAcknowledgedTransfer)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	const auto recipient = identify(server, client);
	ASSERT_TRUE(recipient.isConnected());

	const auto transfer = server.sendTo(recipient, makeMessage(151, 120000));
	auto received = NetworkTestUtils::collect(client.messages(), 1, 5s);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().type(), 151u);
	EXPECT_TRUE(finished(transfer, spk::Transfer::Status::Success));
	EXPECT_NE(transfer.id(), spk::Transfer::InvalidID);
	EXPECT_EQ(transfer.reason(), spk::Transfer::Reason::None);

	client.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, BroadcastReportsEachRecipient)
{
	spk::Server server;
	spk::Client first;
	spk::Client second;
	server.start(0);
	const auto firstConnection = identify(server, first);
	const auto secondConnection = identify(server, second);
	ASSERT_TRUE(firstConnection.isConnected());
	ASSERT_TRUE(secondConnection.isConnected());

	const auto transfers = server.sendToAll(makeMessage(152, 120000));
	ASSERT_EQ(transfers.size(), 2u);
	EXPECT_EQ(NetworkTestUtils::collect(first.messages(), 1, 5s).size(), 1u);
	EXPECT_EQ(NetworkTestUtils::collect(second.messages(), 1, 5s).size(), 1u);
	EXPECT_TRUE(finished(transfers.at(firstConnection.id()), spk::Transfer::Status::Success));
	EXPECT_TRUE(finished(transfers.at(secondConnection.id()), spk::Transfer::Status::Success));

	first.disconnect();
	second.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, RejectDoesNotPartiallyAdmitBroadcast)
{
	spk::Server server;
	spk::Client first;
	spk::Client second;
	server.start(0);
	const auto firstConnection = identify(server, first);
	const auto secondConnection = identify(server, second);
	ASSERT_TRUE(firstConnection.isConnected());
	ASSERT_TRUE(secondConnection.isConnected());
	firstConnection.setMaxQueuedBytes(1);

	const auto transfers = server.sendToAll(makeMessage(153, 100));
	ASSERT_EQ(transfers.size(), 2u);
	EXPECT_TRUE(finished(transfers.at(firstConnection.id()), spk::Transfer::Status::Rejected));
	EXPECT_TRUE(finished(transfers.at(secondConnection.id()), spk::Transfer::Status::Rejected));
	EXPECT_EQ(transfers.at(firstConnection.id()).reason(), spk::Transfer::Reason::QueueOverflow);
	EXPECT_TRUE(first.isConnected());
	EXPECT_TRUE(second.isConnected());
	EXPECT_TRUE(NetworkTestUtils::collect(first.messages(), 1, 100ms).empty());
	EXPECT_TRUE(NetworkTestUtils::collect(second.messages(), 1, 100ms).empty());

	first.disconnect();
	second.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, SkipRecipientPreservesEligibleTransfer)
{
	spk::Server server;
	spk::Client first;
	spk::Client second;
	server.start(0);
	const auto firstConnection = identify(server, first);
	const auto secondConnection = identify(server, second);
	ASSERT_TRUE(firstConnection.isConnected());
	ASSERT_TRUE(secondConnection.isConnected());
	firstConnection.setMaxQueuedBytes(1);

	const auto transfers = server.sendToAll(makeMessage(154, 200), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::SkipRecipient);
	ASSERT_EQ(transfers.size(), 2u);
	EXPECT_TRUE(finished(transfers.at(firstConnection.id()), spk::Transfer::Status::Rejected));
	EXPECT_EQ(NetworkTestUtils::collect(second.messages(), 1, 5s).size(), 1u);
	EXPECT_TRUE(finished(transfers.at(secondConnection.id()), spk::Transfer::Status::Success));
	EXPECT_TRUE(first.isConnected());
	EXPECT_TRUE(second.isConnected());

	first.disconnect();
	second.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, DisconnectRecipientLeavesOtherConnectionsUsable)
{
	spk::Server server;
	spk::Client first;
	spk::Client second;
	server.start(0);
	const auto firstConnection = identify(server, first);
	const auto secondConnection = identify(server, second);
	ASSERT_TRUE(firstConnection.isConnected());
	ASSERT_TRUE(secondConnection.isConnected());
	firstConnection.setMaxQueuedBytes(1);

	const auto transfers = server.sendToAll(makeMessage(155, 200), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::DisconnectRecipient);
	EXPECT_TRUE(finished(transfers.at(firstConnection.id()), spk::Transfer::Status::Rejected));
	EXPECT_TRUE(NetworkTestUtils::waitUntil([&] {
		return !first.isConnected();
	}, 5s));
	EXPECT_EQ(NetworkTestUtils::collect(second.messages(), 1, 5s).size(), 1u);
	EXPECT_TRUE(finished(transfers.at(secondConnection.id()), spk::Transfer::Status::Success));
	EXPECT_TRUE(second.isConnected());

	first.disconnect();
	second.disconnect();
	server.stop();
}
