#include "exception.hpp"
#include "network/client.hpp"
#include "network/internal/frame.hpp"
#include "network/internal/handshake.hpp"
#include "network/internal/socket.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <stop_token>
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
	const auto pendingReported = [&] {
		return pending.load();
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil(pendingReported, 3s));
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
	const auto timedOutReported = [&] {
		return timedOut.load();
	};
	EXPECT_TRUE(NetworkTestUtils::waitUntil(timedOutReported, 3s));
	socket.close();
	server.stop();
}

TEST(HandshakeIntegration, ExplicitSkipRecipientBroadcastDoesNotQueuePendingConnection)
{
	spk::Server server;
	spk::Client ready;
	std::atomic_bool pending = false;
	auto contract = server.subscribeToConnectionStatusChange(
		[&](spk::Connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status == spk::Connection::Status::Pending)
			{
				pending = true;
			}
		});
	server.start(0);
	ready.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(ready));
	pending = false;
	auto socket = spk::NetworkInternal::Socket::connectTCP("127.0.0.1", server.port());
	const auto pendingObserved = [&] {
		return pending.load();
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil(pendingObserved, 3s));
	const auto results = server.sendToAll(
		spk::Message::Writer(4).build(),
		spk::Message::Priority::Normal,
		spk::Connection::OverflowPolicy::SkipRecipient);
	EXPECT_EQ(results.size(), 1u);
	const auto received = NetworkTestUtils::collect(ready.messages(), 1, 3s);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().type(), 4u);
	socket.close();
	ready.disconnect();
	server.stop();
}

TEST(HandshakeIntegration, MalformedPeerDoesNotStopHealthyClientOrServer)
{
	using namespace spk::NetworkInternal;
	spk::Server server;
	spk::Client healthy;
	server.start(0);
	healthy.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(healthy));

	std::atomic_bool disconnected = false;
	auto contract = server.subscribeToConnectionStatusChange(
		[&](spk::Connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status == spk::Connection::Status::Disconnected)
			{
				disconnected = true;
			}
		});
	auto socket = Socket::connectTCP("127.0.0.1", server.port());
	socket.sendAll(*Handshake::encode(Handshake::Kind::Hello));
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> acceptance{};
	ASSERT_TRUE(socket.receiveAllUntil(acceptance, std::chrono::steady_clock::now() + 3s, {}));
	ASSERT_TRUE(Handshake::validAcceptance(Handshake::decode(acceptance)));

	std::array<std::byte, FrameHeaderSize> malformed{};
	malformed[0] = std::byte{'S'};
	malformed[1] = std::byte{'P'};
	malformed[2] = std::byte{'K'};
	malformed[3] = std::byte{'N'};
	write16(malformed.data() + 4, ProtocolVersion);
	write32(malformed.data() + 28, MaximumPayloadSize + 1);
	socket.sendAll(malformed);
	const auto peerDisconnected = [&] {
		return disconnected.load();
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil(peerDisconnected, 3s));
	EXPECT_TRUE(server.isRunning());
	EXPECT_TRUE(healthy.isConnected());

	healthy.send(spk::Message::Writer(902).build());
	const auto received = NetworkTestUtils::collect(server.messages(), 1, 3s);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().message.type(), 902u);
	socket.close();
	healthy.disconnect();
	server.stop();
}

TEST(HandshakeIntegration, CancelledHandshakeWriteDoesNotSendData)
{
	using namespace spk::NetworkInternal;
	auto listener = Socket::listenTCP(0);
	auto socket = Socket::connectTCP("127.0.0.1", listener.localPort());
	std::stop_source cancellation;
	cancellation.request_stop();
	auto hello = Handshake::encode(Handshake::Kind::Hello);
	EXPECT_FALSE(socket.sendAllUntil(*hello, std::chrono::steady_clock::now() + 3s, cancellation.get_token()));
	EXPECT_TRUE(socket.valid());
	socket.close();
}

TEST(HandshakeIntegration, NegotiatesDifferentPageLimitsAndReassemblesMessages)
{
	spk::Server server;
	spk::Client client;
	server.setMaxPageBytes(4096);
	client.setMaxPageBytes(8192);
	server.start(0);
	client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));
	spk::Message::Writer writer(320);
	writer.resize(32768);
	const auto transfer = client.send(std::move(writer).build());
	const auto received = NetworkTestUtils::collect(server.messages(), 1, 5s);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().message.size(), 32768u);
	EXPECT_TRUE(NetworkTestUtils::waitUntil(std::bind_front(&spk::Transfer::isComplete, transfer), 5s));
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Success);
	client.disconnect();
	server.stop();
}

TEST(HandshakeIntegration, SkipRecipientBroadcastWithOnlyPendingClientsReturnsEmpty)
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
	const auto pendingObserved = [&] {
		return pending.load();
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil(pendingObserved, 3s));
	EXPECT_THROW(server.sendToAll(spk::Message::Writer(5).build()), spk::Exception);
	const auto results = server.sendToAll(
		spk::Message::Writer(5).build(),
		spk::Message::Priority::Normal,
		spk::Connection::OverflowPolicy::SkipRecipient);
	EXPECT_TRUE(results.empty());
	socket.close();
	server.stop();
}

TEST(HandshakeIntegration, ClientRequiresOnlyHelloAndAcceptedToBecomeConnected)
{
	using namespace spk::NetworkInternal;
	auto listener = Socket::listenTCP(0);
	spk::Client client;
	const auto connection = client.connect("127.0.0.1", listener.localPort());
	EXPECT_EQ(connection.status(), spk::Connection::Status::Pending);
	auto peer = listener.accept();
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> hello{};
	ASSERT_TRUE(peer.receiveAllUntil(hello, std::chrono::steady_clock::now() + 3s, {}));
	ASSERT_TRUE(Handshake::validHello(Handshake::decode(hello)));
	peer.sendAll(*Handshake::encode(Handshake::Kind::Accepted, 42));
	ASSERT_TRUE(NetworkTestUtils::waitConnected(connection));
	EXPECT_EQ(connection.id(), 42u);
	std::array<std::byte, 1> unexpected{};
	EXPECT_FALSE(peer.receiveAllUntil(unexpected, std::chrono::steady_clock::now() + 100ms, {}));
	EXPECT_TRUE(connection.isConnected());
	client.disconnect();
	peer.close();
}

TEST(HandshakeIntegration, ClientObservesExplicitHandshakeRejectionOnOriginalHandle)
{
	using namespace spk::NetworkInternal;
	auto listener = Socket::listenTCP(0);
	spk::Client client;
	const auto connection = client.connect("127.0.0.1", listener.localPort());
	const auto copy = connection;
	auto peer = listener.accept();
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> hello{};
	ASSERT_TRUE(peer.receiveAllUntil(hello, std::chrono::steady_clock::now() + 3s, {}));
	peer.sendAll(*Handshake::encode(Handshake::Kind::Rejected, 0, spk::Connection::Error::VersionMismatch));
	const auto disconnected = [&] {
		return connection.status() == spk::Connection::Status::Disconnected;
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil(disconnected, 3s));
	EXPECT_EQ(connection.error(), spk::Connection::Error::VersionMismatch);
	EXPECT_EQ(copy.error(), spk::Connection::Error::VersionMismatch);
	EXPECT_EQ(copy.id(), spk::Connection::InvalidID);
	EXPECT_FALSE(client.isConnected());
	client.disconnect();
	peer.close();
}

TEST(HandshakeIntegration, ServerCanSynchronouslySendFromConnectedCallback)
{
	spk::Server server;
	spk::Client client;
	std::atomic_bool sent = false;
	auto contract = server.subscribeToConnectionStatusChange(
		[&](spk::Connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status == spk::Connection::Status::Connected)
			{
				const auto transfers = server.sendToAll(spk::Message::Writer(995).build());
				sent = transfers.size() == 1u;
			}
		});
	server.start(0);
	client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));
	const auto callbackSent = [&] {
		return sent.load();
	};
	EXPECT_TRUE(NetworkTestUtils::waitUntil(callbackSent, 3s));
	const auto received = NetworkTestUtils::collect(client.messages(), 1, 3s);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().type(), 995u);
	client.disconnect();
	server.stop();
}

TEST(HandshakeIntegration, ExpiredAndCancelledHostnameLookupsFailBeforeResolving)
{
	using spk::NetworkInternal::Socket;
	std::stop_source cancellation;
	const auto start = std::chrono::steady_clock::now();
	EXPECT_THROW(Socket::connectTCP("sparkle-name-resolution-test.invalid", 12345, start - 1s, {}), spk::Exception);
	cancellation.request_stop();
	EXPECT_THROW(Socket::connectTCP("sparkle-name-resolution-test.invalid", 12345, start + 5s, cancellation.get_token()), spk::Exception);
	EXPECT_LT(std::chrono::steady_clock::now() - start, 1s);
}

TEST(HandshakeIntegration, HostnameLookupCannotBlockBeyondClientAttemptDeadline)
{
	spk::Client client;
	client.setHandshakeTimeout(150ms);
	const auto start = std::chrono::steady_clock::now();
	const auto connection = client.connect("sparkle-name-resolution-test.invalid", 12345);
	const auto disconnected = [&] {
		return connection.status() == spk::Connection::Status::Disconnected;
	};
	EXPECT_TRUE(NetworkTestUtils::waitUntil(disconnected, 2s));
	EXPECT_LT(std::chrono::steady_clock::now() - start, 2s);
	EXPECT_FALSE(client.isConnected());
	client.disconnect();
}

TEST(HandshakeIntegration, DisconnectDuringHandshakeCannotStartWorkersAfterShutdown)
{
	spk::Server server;
	server.start(0);
	for (std::size_t attempt = 0; attempt < 96; ++attempt)
	{
		spk::Client client;
		const auto connection = client.connect("127.0.0.1", server.port());
		std::jthread stopper([&client] {
			client.disconnect();
		});
		stopper.join();
		EXPECT_EQ(connection.status(), spk::Connection::Status::Disconnected);
		EXPECT_FALSE(client.isConnected());
	}
	server.stop();
}

TEST(HandshakeIntegration, NestedConnectionCallbackDisconnectCannotPublishStaleConnectedEvent)
{
	using spk::NetworkInternal::FrameHeaderSize;
	using spk::NetworkInternal::Handshake;
	using spk::NetworkInternal::Socket;
	auto listener = Socket::listenTCP(0);
	spk::Client client;
	std::atomic_uint32_t staleConnected = 0;
	auto statusContract = client.subscribeToConnectionStatusChange(
		[&](spk::Connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status == spk::Connection::Status::Connected)
			{
				++staleConnected;
			}
		});
	const auto connection = client.connect("127.0.0.1", listener.localPort());
	auto connectionContract = connection.subscribeToStatusChange(
		[&](spk::Connection::Status status, spk::Connection::Error) {
			if (status == spk::Connection::Status::Connected)
			{
				client.disconnect();
			}
		});
	auto peer = listener.accept();
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> hello{};
	ASSERT_TRUE(peer.receiveAllUntil(hello, std::chrono::steady_clock::now() + 3s, {}));
	peer.sendAll(*Handshake::encode(Handshake::Kind::Accepted, 73));
	const auto disconnected = [&] {
		return connection.status() == spk::Connection::Status::Disconnected;
	};
	ASSERT_TRUE(NetworkTestUtils::waitUntil(disconnected, 3s));
	client.disconnect();
	EXPECT_EQ(staleConnected.load(), 0u);
	peer.close();
}
