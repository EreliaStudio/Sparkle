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

TEST(ServerTransferIntegration, ExplicitSingleTargetSkipReturnsRejectedTransfer)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	const auto recipient = identify(server, client);
	ASSERT_TRUE(recipient.isConnected());
	recipient.setMaxQueuedBytes(1);
	const auto transfer = server.sendTo(recipient, makeMessage(156, 200), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::SkipRecipient);
	EXPECT_TRUE(finished(transfer, spk::Transfer::Status::Rejected));
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
	EXPECT_THROW(server.sendTo(recipient, makeMessage(161, 16), spk::Message::Priority::Normal, invalid), spk::Exception);
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
	EXPECT_TRUE(server.sendToAll(makeMessage(163, 16)).empty());
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

TEST(ServerTransferIntegration, LoweringOutgoingLimitPreservesAcceptedTransfer)
{
	spk::Server server;
	spk::Client client;
	server.setGlobalMaxQueuedBytes(512u * 1024u);
	server.start(0);
	const auto recipient = identify(server, client);
	ASSERT_TRUE(recipient.isConnected());

	const auto accepted = server.sendTo(recipient, makeMessage(197, 128u * 1024u));
	server.setGlobalMaxQueuedBytes(1);
	EXPECT_THROW(server.sendTo(recipient, makeMessage(198, 1)), spk::Exception);
	auto received = NetworkTestUtils::collect(client.messages(), 1, 10s);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().type(), 197u);
	EXPECT_TRUE(finished(accepted, spk::Transfer::Status::Success));

	server.setGlobalMaxQueuedBytes(4096);
	const auto resumed = server.sendTo(recipient, makeMessage(199, 1));
	EXPECT_EQ(NetworkTestUtils::collect(client.messages(), 1, 5s).size(), 1u);
	EXPECT_TRUE(finished(resumed, spk::Transfer::Status::Success));
	client.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, ExplicitTargetSetSkipReportsSaturatedRecipientWithoutBlockingHealthyPeer)
{
	spk::Server server;
	spk::Client first;
	spk::Client second;
	server.start(0);
	auto saturated = identify(server, first);
	const auto healthy = identify(server, second);
	ASSERT_TRUE(saturated.isConnected());
	ASSERT_TRUE(healthy.isConnected());
	saturated.setMaxQueuedBytes(1);
	const spk::ConnectionSet targets{saturated, healthy};

	const auto results = server.sendTo(
		targets,
		makeMessage(200, 64),
		spk::Message::Priority::Normal,
		spk::Connection::OverflowPolicy::SkipRecipient);
	ASSERT_EQ(results.size(), 2u);
	EXPECT_TRUE(finished(results.at(saturated.id()), spk::Transfer::Status::Rejected));
	EXPECT_TRUE(finished(results.at(healthy.id()), spk::Transfer::Status::Success));
	EXPECT_EQ(NetworkTestUtils::collect(second.messages(), 1, 5s).size(), 1u);
	EXPECT_TRUE(NetworkTestUtils::collect(first.messages(), 1, 100ms).empty());
	EXPECT_TRUE(first.isConnected());
	EXPECT_TRUE(second.isConnected());
	first.disconnect();
	second.disconnect();
	server.stop();
}

TEST(ServerTransferIntegration, EmptyBroadcastNeedsNoCommandCapacityForEveryPolicy)
{
	spk::Server server;
	server.setGlobalMaxQueuedBytes(1);
	server.start(0);
	std::vector<std::future<spk::Server::TransferResults>> calls;
	for (std::uint32_t index = 0; index < 8; ++index)
	{
		calls.push_back(std::async(std::launch::async, [&server, index] {
			return server.sendToAll(makeMessage(210 + index, 128), spk::Message::Priority::Normal, static_cast<spk::Connection::OverflowPolicy>(index % 3));
		}));
	}
	for (auto &call : calls)
	{
		ASSERT_EQ(call.wait_for(3s), std::future_status::ready);
		EXPECT_TRUE(call.get().empty());
	}
	EXPECT_TRUE(server.sendToAll(makeMessage(220, 128)).empty());
	EXPECT_TRUE(server.sendToAll(makeMessage(220, 128), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::DisconnectRecipient).empty());
	EXPECT_THROW(server.sendToAll(makeMessage(220, 128), spk::Message::Priority::Normal, static_cast<spk::Connection::OverflowPolicy>(99)), spk::Exception);
	EXPECT_THROW(server.sendToAll(makeMessage(220, 128), static_cast<spk::Message::Priority>(99), spk::Connection::OverflowPolicy::SkipRecipient), spk::Exception);
	server.stop();
	EXPECT_THROW(server.sendToAll(makeMessage(220, 128), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::SkipRecipient), spk::Exception);
}

TEST(ServerTransferIntegration, AllPendingSkipBroadcastNeedsNoCommandCapacity)
{
	using namespace spk::NetworkInternal;
	spk::ThreadSafeFIFO<spk::Connection> pending;
	spk::Server server;
	auto contract = server.subscribeToConnectionStatusChange([&](spk::Connection connection,
																 spk::Connection::Status status,
																 spk::Connection::Error) {
		if (status == spk::Connection::Status::Pending)
		{
			pending.publish(connection);
		}
	});
	server.start(0);
	auto first = Socket::connectTCP("127.0.0.1", server.port());
	auto second = Socket::connectTCP("127.0.0.1", server.port());
	const auto connections = NetworkTestUtils::collect(pending, 2, 3s);
	ASSERT_EQ(connections.size(), 2u);
	server.setGlobalMaxQueuedBytes(1);
	EXPECT_TRUE(server.sendToAll(makeMessage(221, 128), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::SkipRecipient).empty());
	EXPECT_EQ(connections[0].status(), spk::Connection::Status::Pending);
	EXPECT_EQ(connections[1].status(), spk::Connection::Status::Pending);
	EXPECT_THROW(server.sendTo(connections[0], makeMessage(221, 128)), spk::Exception);
	EXPECT_THROW(server.sendToAll(makeMessage(221, 128)), spk::Exception);
	EXPECT_THROW(server.sendToAll(makeMessage(221, 128), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::DisconnectRecipient), spk::Exception);
	first.setNonBlocking(true);
	second.setNonBlocking(true);
	std::array<std::byte, 1> buffer{};
	EXPECT_FALSE(first.receiveSome(buffer).has_value());
	EXPECT_FALSE(second.receiveSome(buffer).has_value());
	first.close();
	second.close();
	server.stop();
}

TEST(ServerTransferIntegration, SkipBroadcastTracksReadinessDisconnectAndRestart)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	const auto connection = identify(server, client);
	ASSERT_TRUE(connection.isConnected());
	server.setGlobalMaxQueuedBytes(1);
	EXPECT_THROW(server.sendToAll(makeMessage(222, 128), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::SkipRecipient), spk::Exception);
	server.setGlobalMaxQueuedBytes(4096);
	const auto results = server.sendToAll(makeMessage(223, 128), spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::SkipRecipient);
	ASSERT_EQ(results.size(), 1u);
	EXPECT_TRUE(finished(results.at(connection.id()), spk::Transfer::Status::Success));
	EXPECT_EQ(NetworkTestUtils::collect(client.messages(), 1, 3s).size(), 1u);
	client.disconnect();
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		return !connection.isConnected();
	}));
	server.setGlobalMaxQueuedBytes(1);
	EXPECT_TRUE(server.sendToAll(makeMessage(224, 128)).empty());
	server.stop();
	server.start(0);
	EXPECT_TRUE(server.sendToAll(makeMessage(225, 128)).empty());
	server.stop();
}

TEST(ServerTransferIntegration, EmptyBroadcastFromDisconnectionCallbackReturnsEmpty)
{
	std::promise<spk::Server::TransferResults> completed;
	auto result = completed.get_future();
	spk::Server server;
	spk::Client client;
	auto contract = server.subscribeToConnectionStatusChange([&](spk::Connection, spk::Connection::Status status, spk::Connection::Error) {
		if (status == spk::Connection::Status::Disconnected)
		{
			completed.set_value(server.sendToAll(makeMessage(226, 128)));
		}
	});
	server.start(0);
	ASSERT_TRUE(identify(server, client).isConnected());
	server.setGlobalMaxQueuedBytes(1);
	client.disconnect();
	ASSERT_EQ(result.wait_for(3s), std::future_status::ready);
	EXPECT_TRUE(result.get().empty());
	server.stop();
}
