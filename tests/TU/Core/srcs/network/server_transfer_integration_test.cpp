#include "exception.hpp"
#include "network/client.hpp"
#include "network/internal/handshake.hpp"
#include "network/internal/socket.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <thread>
#include <utility>
#include <vector>

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
		return NetworkTestUtils::waitUntil(
			[&] {
				return transfer.status() == status;
			},
			5s);
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
	auto firstConnection = identify(server, first);
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
	auto firstConnection = identify(server, first);
	const auto secondConnection = identify(server, second);
	ASSERT_TRUE(firstConnection.isConnected());
	ASSERT_TRUE(secondConnection.isConnected());
	firstConnection.setMaxQueuedBytes(1);

	EXPECT_THROW(server.sendToAll(makeMessage(153, 100), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::Reject), spk::Exception);
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
	auto firstConnection = identify(server, first);
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
	auto firstConnection = identify(server, first);
	const auto secondConnection = identify(server, second);
	ASSERT_TRUE(firstConnection.isConnected());
	ASSERT_TRUE(secondConnection.isConnected());
	firstConnection.setMaxQueuedBytes(1);

	const auto transfers = server.sendToAll(makeMessage(155, 200), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::DisconnectRecipient);
	EXPECT_TRUE(finished(transfers.at(firstConnection.id()), spk::Transfer::Status::Rejected));
	EXPECT_TRUE(NetworkTestUtils::waitUntil([&] {
		return !first.isConnected();
	},
											5s));
	EXPECT_EQ(NetworkTestUtils::collect(second.messages(), 1, 5s).size(), 1u);
	EXPECT_TRUE(finished(transfers.at(secondConnection.id()), spk::Transfer::Status::Success));
	EXPECT_TRUE(second.isConnected());

	first.disconnect();
	second.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, TargetedSendThrowsImmediatelyWhenRecipientCannotFitMessage)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	auto recipient = identify(server, client);
	ASSERT_TRUE(recipient.isConnected());
	recipient.setMaxQueuedBytes(1);
	EXPECT_THROW(server.sendTo(recipient, makeMessage(156, 200)), spk::Exception);
	EXPECT_TRUE(client.isConnected());
	EXPECT_TRUE(NetworkTestUtils::collect(client.messages(), 1, 100ms).empty());
	client.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, InvalidPriorityThrowsBeforeScheduling)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	const auto recipient = identify(server, client);
	ASSERT_TRUE(recipient.isConnected());
	EXPECT_THROW(server.sendTo(recipient, makeMessage(157, 1), static_cast<spk::Message::Priority>(99)), spk::Exception);
	client.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, RejectsImpossibleGlobalCommandQueueAdmissionSynchronously)
{
	spk::Server server;
	spk::Client client;
	server.setGlobalMaxQueuedBytes(1024);
	server.start(0);
	const auto recipient = identify(server, client);
	ASSERT_TRUE(recipient.isConnected());
	EXPECT_THROW(server.sendTo(recipient, makeMessage(158, 1025)), spk::Exception);
	EXPECT_TRUE(client.isConnected());
	const auto transfer = server.sendTo(recipient, makeMessage(159, 2));
	EXPECT_EQ(NetworkTestUtils::collect(client.messages(), 1, 5s).size(), 1u);
	EXPECT_TRUE(finished(transfer, spk::Transfer::Status::Success));
	client.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, InvalidOverflowPolicyThrowsWithoutSending)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	const auto recipient = identify(server, client);
	ASSERT_TRUE(recipient.isConnected());
	const auto invalid = static_cast<spk::Connection::OverflowPolicy>(99);
	EXPECT_THROW(server.sendToAll(makeMessage(160, 16), spk::Message::Priority::Normal, invalid), spk::Exception);
	EXPECT_THROW(server.sendTo(spk::ConnectionSet{recipient}, makeMessage(161, 16), spk::Message::Priority::Normal, invalid), spk::Exception);
	EXPECT_TRUE(NetworkTestUtils::collect(client.messages(), 1, 100ms).empty());
	client.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, ExplicitSkipRecipientCanBroadcastToEmptyAudience)
{
	spk::Server server;
	server.start(0);
	const auto results = server.sendToAll(
		makeMessage(162, 16),
		spk::Message::Priority::Normal,
		spk::Connection::OverflowPolicy::SkipRecipient);
	EXPECT_TRUE(results.empty());
	EXPECT_THROW(server.sendToAll(makeMessage(163, 16)), spk::Exception);
	server.stop();
}

TEST(ServerTransferIntegration, RejectReservesLogicalAndFrameBytesBeforeReturning)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	const auto recipient = identify(server, client);
	ASSERT_TRUE(recipient.isConnected());
	server.setGlobalMaxQueuedBytes(80);
	EXPECT_THROW(server.sendTo(recipient, makeMessage(170, 16)), spk::Exception);
	EXPECT_TRUE(NetworkTestUtils::collect(client.messages(), 1, 100ms).empty());
	const auto transfer = server.sendTo(recipient, makeMessage(171, 1));
	EXPECT_EQ(NetworkTestUtils::collect(client.messages(), 1, 5s).size(), 1u);
	EXPECT_TRUE(finished(transfer, spk::Transfer::Status::Success));
	client.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, ConcurrentBroadcastAdmissionsReturnBothCommittedRecipients)
{
	spk::Server server;
	spk::Client first;
	spk::Client second;
	server.start(0);
	ASSERT_TRUE(identify(server, first).isConnected());
	ASSERT_TRUE(identify(server, second).isConnected());
	std::vector<std::future<spk::Server::TransferResults>> calls;
	for (std::uint32_t index = 0; index < 4; ++index)
	{
		calls.push_back(std::async(std::launch::async, [&server, index] {
			return server.sendToAll(makeMessage(180 + index, 512));
		}));
	}
	for (auto &call : calls)
	{
		const auto transfers = call.get();
		ASSERT_EQ(transfers.size(), 2u);
		for (const auto &[id, transfer] : transfers)
		{
			(void)id;
			EXPECT_TRUE(finished(transfer, spk::Transfer::Status::Success));
		}
	}
	EXPECT_EQ(NetworkTestUtils::collect(first.messages(), 4, 5s).size(), 4u);
	EXPECT_EQ(NetworkTestUtils::collect(second.messages(), 4, 5s).size(), 4u);
	first.disconnect();
	second.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, SharedGlobalBudgetRejectsWholeBroadcastBeforeCommit)
{
	spk::Server server;
	spk::Client first;
	spk::Client second;
	server.start(0);
	ASSERT_TRUE(identify(server, first).isConnected());
	ASSERT_TRUE(identify(server, second).isConnected());
	server.setGlobalMaxQueuedBytes(400);
	EXPECT_THROW(server.sendToAll(makeMessage(190, 80)), spk::Exception);
	EXPECT_TRUE(NetworkTestUtils::collect(first.messages(), 1, 100ms).empty());
	EXPECT_TRUE(NetworkTestUtils::collect(second.messages(), 1, 100ms).empty());
	server.setGlobalMaxQueuedBytes(4096);
	const auto results = server.sendToAll(makeMessage(191, 80));
	ASSERT_EQ(results.size(), 2u);
	EXPECT_EQ(NetworkTestUtils::collect(first.messages(), 1, 5s).size(), 1u);
	EXPECT_EQ(NetworkTestUtils::collect(second.messages(), 1, 5s).size(), 1u);
	for (const auto &[id, transfer] : results)
	{
		(void)id;
		EXPECT_TRUE(finished(transfer, spk::Transfer::Status::Success));
	}
	first.disconnect();
	second.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, ShutdownUnblocksConcurrentAdmissionCallers)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	ASSERT_TRUE(identify(server, client).isConnected());
	std::vector<std::future<void>> calls;
	for (std::size_t index = 0; index < 12; ++index)
	{
		calls.push_back(std::async(std::launch::async, [&server] {
			try
			{
				(void)server.sendToAll(makeMessage(192, 256));
			} catch (const spk::Exception &)
			{
			}
		}));
	}
	server.stop();
	for (auto &call : calls)
	{
		ASSERT_EQ(call.wait_for(5s), std::future_status::ready);
		EXPECT_NO_THROW(call.get());
	}
	client.disconnect();
}

TEST(ServerTransferIntegration, DisconnectReclaimsOutgoingMemoryForNextClient)
{
	using namespace spk::NetworkInternal;
	spk::Server server;
	server.setGlobalMaxQueuedBytes(3u * 1024u * 1024u);
	server.start(0);
	auto peer = Socket::connectTCP("127.0.0.1", server.port());
	peer.sendAll(*Handshake::encode(Handshake::Kind::Hello));
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> response{};
	ASSERT_TRUE(peer.receiveAllUntil(response, std::chrono::steady_clock::now() + 3s, {}));
	const auto accepted = Handshake::decode(response);
	ASSERT_TRUE(Handshake::validAcceptance(accepted));
	const auto recipient = server.connection(accepted.connectionID);
	ASSERT_TRUE(NetworkTestUtils::waitConnected(recipient));
	auto abandoned = server.sendTo(recipient, makeMessage(195, 2u * 1024u * 1024u));
	peer.close();
	ASSERT_TRUE(NetworkTestUtils::waitUntil(std::bind_front(&spk::Transfer::isComplete, abandoned), 5s));
	EXPECT_EQ(abandoned.status(), spk::Transfer::Status::Indeterminate);

	spk::Client healthy;
	const auto current = identify(server, healthy);
	ASSERT_TRUE(current.isConnected());
	const auto transfer = server.sendTo(current, makeMessage(196, 2u * 1024u * 1024u));
	EXPECT_EQ(NetworkTestUtils::collect(healthy.messages(), 1, 10s).size(), 1u);
	EXPECT_TRUE(finished(transfer, spk::Transfer::Status::Success));
	healthy.disconnect();
	server.stop();
}
