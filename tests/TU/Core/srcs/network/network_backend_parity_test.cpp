#include "network/client.hpp"
#include "network/internal/handshake.hpp"
#include "network/internal/page.hpp"
#include "network/internal/socket.hpp"
#include "network/internal/transfer_response.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <optional>
#include <span>
#include <stop_token>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	using spk::NetworkInternal::Handshake;
	using spk::NetworkInternal::Socket;

	[[nodiscard]] std::vector<std::byte> receiveFrame(Socket &socket)
	{
		const auto deadline = std::chrono::steady_clock::now() + 5s;
		std::array<std::byte, spk::NetworkInternal::FrameHeaderSize> header{};
		if (!socket.receiveAllUntil(header, deadline, {}))
		{
			throw spk::Exception("Test peer did not receive a frame header.");
		}
		const auto decoded = spk::NetworkInternal::decode(header);
		std::vector<std::byte> frame(header.begin(), header.end());
		frame.resize(header.size() + decoded.payloadSize);
		if (!socket.receiveAllUntil(std::span(frame).subspan(header.size()), deadline, {}))
		{
			throw spk::Exception("Test peer did not receive a frame payload.");
		}
		return frame;
	}

	[[nodiscard]] std::pair<Socket, Handshake::Packet> connectPeer(spk::Server &server)
	{
		auto socket = Socket::connectTCP("127.0.0.1", server.port());
		const auto hello = Handshake::encode(Handshake::Kind::Hello);
		for (const auto &byte : *hello)
		{
			socket.sendAll(std::span(&byte, 1));
		}
		const auto acceptance = Handshake::decode(receiveFrame(socket));
		if (!Handshake::validAcceptance(acceptance))
		{
			throw spk::Exception("Test peer handshake was rejected.");
		}
		return {std::move(socket), acceptance};
	}
}

TEST(NetworkBackendParity, FragmentedAndCoalescedFramesPreserveMessagesAndAcknowledgments)
{
	spk::Server server;
	server.start(0);
	auto [socket, acceptance] = connectPeer(server);
	spk::Message::Writer legacyWriter(401);
	legacyWriter << std::uint32_t{17};
	const auto legacy = spk::NetworkInternal::encode(std::move(legacyWriter).build());
	spk::Message::Writer pagedWriter(402);
	pagedWriter << std::uint32_t{29};
	const auto paged = std::move(pagedWriter).build();
	const auto page = spk::NetworkInternal::encodePage({paged, 1, paged.size(), 0, paged.size()});
	std::vector<std::byte> bytes(*legacy);
	bytes.insert(bytes.end(), page->begin(), page->end());
	const std::size_t split = legacy->size() + 7;
	socket.sendAll(std::span(bytes).first(split));
	const auto first = NetworkTestUtils::collect(server.messages(), 1, 5s);
	ASSERT_EQ(first.size(), 1u);
	EXPECT_EQ(first.front().message.type(), 401u);
	socket.sendAll(std::span(bytes).subspan(split));
	const auto second = NetworkTestUtils::collect(server.messages(), 1, 5s);
	ASSERT_EQ(second.size(), 1u);
	EXPECT_EQ(second.front().message.type(), 402u);
	EXPECT_EQ(second.front().message.connectionID(), acceptance.connectionID);
	EXPECT_EQ(second.front().message.reader().get<std::uint32_t>(), 29u);
	const auto response = spk::NetworkInternal::TransferResponse::decode(receiveFrame(socket));
	EXPECT_EQ(response.transferID, 1u);
	EXPECT_EQ(response.status, spk::Transfer::Status::Success);
	EXPECT_EQ(response.connectionID, acceptance.connectionID);
}

TEST(NetworkBackendParity, ReadyCallbackCanSendBeforeTheNextKernelEvent)
{
	spk::Server server;
	spk::Client client;
	std::promise<spk::Transfer> sent;
	auto result = sent.get_future();
	auto contract = server.subscribeToConnectionStatusChange(
		[&](spk::Connection connection, spk::Connection::Status status, spk::Connection::Error) {
			if (status == spk::Connection::Status::Connected)
			{
				sent.set_value(server.sendTo(connection, spk::Message::Writer(403).build()));
			}
		});
	server.start(0);
	client.connect("127.0.0.1", server.port());
	ASSERT_EQ(result.wait_for(5s), std::future_status::ready);
	const auto transfer = result.get();
	const auto received = NetworkTestUtils::collect(client.messages(), 1, 5s);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().type(), 403u);
	EXPECT_TRUE(NetworkTestUtils::waitUntil([&] {
		return transfer.isComplete();
	},
											5s));
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Success);
}

TEST(NetworkBackendParity, RuntimeSettingsReachExistingAndNewSessions)
{
	spk::Server server;
	spk::Client first;
	spk::Client second;
	server.start(0);
	first.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(first));
	server.setMaxPageBytes(3);
	server.setMaxMessageBytes(4096);
	server.setMaxIncomingBytes(8192);
	server.setIncomingTransferTimeout(5s);
	server.setAcknowledgmentTimeout(5s);
	second.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(second));
	spk::Message::Writer writer(404);
	writer << std::uint64_t{0x1020304050607080};
	const auto message = std::move(writer).build();
	const auto outgoing = server.sendToAll(message);
	ASSERT_EQ(outgoing.size(), 2u);
	for (spk::Client *client : {&first, &second})
	{
		const auto received = NetworkTestUtils::collect(client->messages(), 1, 5s);
		ASSERT_EQ(received.size(), 1u);
		EXPECT_EQ(received.front().data().size(), message.size());
		EXPECT_EQ(received.front().reader().get<std::uint64_t>(), 0x1020304050607080u);
		const auto incoming = client->send(message);
		EXPECT_TRUE(NetworkTestUtils::waitUntil([&] {
			return incoming.isComplete();
		},
												5s));
		EXPECT_EQ(incoming.status(), spk::Transfer::Status::Success);
	}
	const auto received = NetworkTestUtils::collect(server.messages(), 2, 5s);
	EXPECT_EQ(received.size(), 2u);
	for (const auto &[id, transfer] : outgoing)
	{
		EXPECT_TRUE(NetworkTestUtils::waitUntil([&] {
			return transfer.isComplete();
		},
												5s));
		EXPECT_EQ(transfer.status(), spk::Transfer::Status::Success);
	}
}

TEST(NetworkSocket, NonblockingReceiveDistinguishesWaitingDataAndDisconnection)
{
	auto listener = Socket::listenTCP(0);
	auto client = Socket::connectTCP("127.0.0.1", listener.localPort());
	auto peer = listener.accept();
	peer.setNonBlocking(true);
	std::array<std::byte, 4> buffer{};
	EXPECT_FALSE(peer.receiveSome(buffer).has_value());
	const std::array<std::byte, 4> payload{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
	ASSERT_EQ(client.sendSome(payload), payload.size());
	ASSERT_TRUE(peer.receiveAllUntil(buffer, std::chrono::steady_clock::now() + 5s, {}));
	EXPECT_EQ(buffer, payload);
	peer.setNonBlocking(true);
	client.close();
	std::optional<Socket::ReceiveResult> result;
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		result = peer.receiveSome(buffer);
		return result.has_value();
	},
											5s));
	EXPECT_TRUE(result->disconnected);
	EXPECT_EQ(result->size, 0u);
}

TEST(NetworkSocket, EmptyBuffersDoNotReportDisconnection)
{
	Socket socket;
	EXPECT_EQ(socket.sendSome({}), 0u);
	const auto received = socket.receiveSome({});
	ASSERT_TRUE(received.has_value());
	EXPECT_FALSE(received->disconnected);
	EXPECT_EQ(received->size, 0u);
}

TEST(NetworkSocket, CanceledDeadlineReceiveCanBeFollowedByAnotherReceive)
{
	auto listener = Socket::listenTCP(0);
	auto client = Socket::connectTCP("127.0.0.1", listener.localPort());
	auto peer = listener.accept();
	std::stop_source cancellation;
	cancellation.request_stop();
	std::array<std::byte, 1> buffer{};
	EXPECT_FALSE(peer.receiveAllUntil(buffer, std::chrono::steady_clock::now() + 5s, cancellation.get_token()));
	const std::array<std::byte, 1> payload{std::byte{42}};
	client.sendAll(payload);
	EXPECT_TRUE(peer.receiveAll(buffer));
	EXPECT_EQ(buffer, payload);
}

TEST(NetworkSocket, NonblockingSendReportsBackpressureWithoutDisconnecting)
{
	auto listener = Socket::listenTCP(0);
	auto client = Socket::connectTCP("127.0.0.1", listener.localPort());
	auto peer = listener.accept();
	client.setNonBlocking(true);
	const std::vector<std::byte> payload(1024u * 1024u, std::byte{9});
	std::size_t sentBytes = 0;
	const auto deadline = std::chrono::steady_clock::now() + 5s;
	bool waiting = false;
	while (sentBytes < 64u * 1024u * 1024u && std::chrono::steady_clock::now() < deadline)
	{
		const auto sent = client.sendSome(payload);
		if (!sent.has_value())
		{
			waiting = true;
			break;
		}
		ASSERT_GT(*sent, 0u);
		sentBytes += *sent;
	}
	EXPECT_TRUE(waiting);
	EXPECT_GT(sentBytes, 0u);
	EXPECT_TRUE(client.valid());
	std::array<std::byte, 1> received{};
	EXPECT_TRUE(peer.receiveAllUntil(received, deadline, {}));
	EXPECT_EQ(received.front(), std::byte{9});
}
