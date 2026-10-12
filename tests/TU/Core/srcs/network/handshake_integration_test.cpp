#include "exception.hpp"
#include "network/client.hpp"
#include "network/internal/handshake.hpp"
#include "network/internal/socket.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <utility>

using namespace std::chrono_literals;

TEST(HandshakeIntegration, PendingClientRejectsApplicationMessagesAndTimesOut)
{
	auto listener = spk::NetworkInternal::Socket::listenTCP(0);
	spk::Client client;
	client.setHandshakeTimeout(500ms);
	spk::Connection connection = client.connect("127.0.0.1", listener.localPort());
	EXPECT_EQ(connection.status(), spk::Connection::Status::Pending);
	EXPECT_EQ(connection.id(), spk::Connection::InvalidID);
	EXPECT_THROW(client.send(spk::Message::Writer(1).build()), spk::Exception);
	const auto disconnected = [&] {
		return connection.status() == spk::Connection::Status::Disconnected;
	};
	EXPECT_TRUE(NetworkTestUtils::waitUntil(disconnected, 3s));
	EXPECT_EQ(connection.error(), spk::Connection::Error::Timeout);
	EXPECT_FALSE(client.isConnected());
	client.disconnect();
}

TEST(HandshakeIntegration, ServerRejectsTargetedSendDuringPending)
{
	spk::Server server;
	spk::Client client;
	std::atomic_bool rejected = false;
	auto contract = server.subscribeToConnectionStatusChange(
		[&](spk::Connection connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status != spk::Connection::Status::Pending)
			{
				return;
			}
			try
			{
				server.sendTo(connection, spk::Message::Writer(2).build());
			} catch (const spk::Exception &)
			{
				rejected = true;
			}
		});
	server.start(0);
	spk::Connection connection = client.connect("127.0.0.1", server.port());
	EXPECT_TRUE(NetworkTestUtils::waitConnected(connection));
	EXPECT_TRUE(rejected.load());
	EXPECT_NE(connection.id(), spk::Connection::InvalidID);
	client.disconnect();
	server.stop();
}

TEST(HandshakeIntegration, IncompatibleVersionReceivesExplicitRejection)
{
	spk::Server server;
	server.start(0);
	auto socket = spk::NetworkInternal::Socket::connectTCP("127.0.0.1", server.port());
	auto hello = spk::NetworkInternal::Handshake::encode(spk::NetworkInternal::Handshake::Kind::Hello);
	(*hello)[37] = std::byte{2};
	socket.sendAll(*hello);
	std::array<std::byte, spk::NetworkInternal::FrameHeaderSize + spk::NetworkInternal::Handshake::PayloadSize> received{};
	ASSERT_TRUE(socket.receiveAll(received));
	const auto response = spk::NetworkInternal::Handshake::decode(received);
	EXPECT_EQ(response.kind, spk::NetworkInternal::Handshake::Kind::Rejected);
	EXPECT_EQ(response.error, spk::Connection::Error::VersionMismatch);
	socket.close();
	server.stop();
}

TEST(HandshakeIntegration, ReconnectRequiresExplicitDisconnect)
{
	auto listener = spk::NetworkInternal::Socket::listenTCP(0);
	spk::Client client;
	auto abandoned = client.connect("127.0.0.1", listener.localPort());
	EXPECT_EQ(abandoned.status(), spk::Connection::Status::Pending);
	spk::Server server;
	server.start(0);
	EXPECT_THROW(client.connect("127.0.0.1", server.port()), spk::Exception);
	EXPECT_EQ(abandoned.status(), spk::Connection::Status::Pending);
	client.disconnect();
	auto connected = client.connect("127.0.0.1", server.port());
	EXPECT_NE(abandoned, connected);
	EXPECT_EQ(abandoned.status(), spk::Connection::Status::Disconnected);
	EXPECT_TRUE(NetworkTestUtils::waitConnected(connected));
	EXPECT_FALSE(abandoned.isConnected());
	client.disconnect();
	server.stop();
}

TEST(HandshakeIntegration, ServerBroadcastRejectsPendingRecipient)
{
	spk::Server server;
	std::atomic_bool pending = false;
	auto contract = server.subscribeToConnectionStatusChange(
		[&](spk::Connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status == spk::Connection::Status::Pending)
			{
				pending = true;
			}
		});
	server.start(0);
	auto socket = spk::NetworkInternal::Socket::connectTCP("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		return pending.load();
	}, 3s));
	EXPECT_THROW(server.sendToAll(spk::Message::Writer(3).build()), spk::Exception);
	socket.close();
	server.stop();
}

TEST(HandshakeIntegration, ServerUsesTimeoutConfiguredBeforeStart)
{
	spk::Server server;
	std::atomic_bool timedOut = false;
	auto contract = server.subscribeToConnectionStatusChange(
		[&](spk::Connection, spk::Connection::Status status, spk::Connection::Error error) {
			if (status == spk::Connection::Status::Disconnected && error == spk::Connection::Error::Timeout)
			{
				timedOut = true;
			}
		});
	server.setHandshakeTimeout(100ms);
	server.start(0);
	auto socket = spk::NetworkInternal::Socket::connectTCP("127.0.0.1", server.port());
	EXPECT_TRUE(NetworkTestUtils::waitUntil([&] {
		return timedOut.load();
	}, 3s));
	socket.close();
	server.stop();
}
