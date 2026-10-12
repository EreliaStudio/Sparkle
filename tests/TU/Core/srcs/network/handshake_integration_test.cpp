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
	EXPECT_TRUE(NetworkTestUtils::waitUntil([&] {
		return connection.status() == spk::Connection::Status::Disconnected;
	}, 3s));
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
	EXPECT_TRUE(NetworkTestUtils::waitUntil([&] {
		return connection.isConnected();
	}, 5s));
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

TEST(HandshakeIntegration, ReconnectReplacesPendingSessionWithoutRevivingIt)
{
	auto listener = spk::NetworkInternal::Socket::listenTCP(0);
	spk::Client client;
	auto abandoned = client.connect("127.0.0.1", listener.localPort());
	EXPECT_EQ(abandoned.status(), spk::Connection::Status::Pending);
	spk::Server server;
	server.start(0);
	auto connected = client.connect("127.0.0.1", server.port());
	EXPECT_NE(abandoned, connected);
	EXPECT_EQ(abandoned.status(), spk::Connection::Status::Disconnected);
	EXPECT_TRUE(NetworkTestUtils::waitConnected(connected));
	EXPECT_FALSE(abandoned.isConnected());
	client.disconnect();
	server.stop();
}
