#include "exception.hpp"
#include "network/client.hpp"
#include "network/internal/handshake.hpp"
#include "network/internal/incoming_frame_validator.hpp"
#include "network/internal/page_scheduler.hpp"
#include "network/internal/socket.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <utility>

namespace
{
	[[nodiscard]] spk::Message makeMessage(std::size_t bytes)
	{
		spk::Message::Writer writer(201);
		writer.resize(bytes);
		return std::move(writer).build();
	}
}

TEST(TransportLimits, RejectsZeroSizedConfiguration)
{
	spk::Client client;
	spk::Server server;
	EXPECT_THROW(client.setMaxMessageBytes(0), spk::Exception);
	EXPECT_THROW(client.setMaxIncomingBytes(0), spk::Exception);
	EXPECT_THROW(server.setMaxMessageBytes(0), spk::Exception);
	EXPECT_THROW(server.setMaxIncomingBytes(0), spk::Exception);
	EXPECT_THROW(server.setGlobalMaxIncomingBytes(0), spk::Exception);
	EXPECT_THROW(server.setGlobalMaxQueuedBytes(0), spk::Exception);
	spk::NetworkInternal::PageScheduler scheduler;
	EXPECT_THROW(scheduler.setMaxMessageBytes(0), spk::Exception);
	EXPECT_THROW(client.setMaxPageBytes(0), spk::Exception);
	EXPECT_THROW(server.setMaxPageBytes(0), spk::Exception);
	EXPECT_THROW(client.setMaxPageBytes(spk::NetworkInternal::MaximumSupportedPageBytes + 1), spk::Exception);
	EXPECT_THROW(server.setMaxPageBytes(spk::NetworkInternal::MaximumSupportedPageBytes + 1), spk::Exception);
}

TEST(TransportLimits, OutgoingAndIncomingLimitsApplyToClientAndServer)
{
	spk::Server server;
	spk::Client client;
	server.setMaxMessageBytes(64);
	server.setMaxIncomingBytes(2048);
	server.setGlobalMaxIncomingBytes(4096);
	client.setMaxMessageBytes(64);
	client.setMaxIncomingBytes(2048);
	server.start(0);
	client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));
	EXPECT_THROW(client.send(makeMessage(65)), spk::Exception);
	const auto transfer = client.send(makeMessage(64));
	const auto received = NetworkTestUtils::collect(server.messages(), 1, std::chrono::seconds(5));
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().message.size(), 64u);
	EXPECT_TRUE(NetworkTestUtils::waitUntil(std::bind_front(&spk::Transfer::isComplete, transfer), std::chrono::seconds(5)));
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Success);
	const auto connection = server.connection(received.front().emitter);
	EXPECT_THROW(server.sendTo(connection, makeMessage(65)), spk::Exception);
	const auto reply = server.sendTo(connection, makeMessage(64));
	EXPECT_EQ(NetworkTestUtils::collect(client.messages(), 1, std::chrono::seconds(5)).size(), 1u);
	EXPECT_TRUE(NetworkTestUtils::waitUntil(std::bind_front(&spk::Transfer::isComplete, reply), std::chrono::seconds(5)));
	EXPECT_EQ(reply.status(), spk::Transfer::Status::Success);
	client.disconnect();
	server.stop();
}

TEST(TransportLimits, ClientOutgoingBudgetCoversFrameHeadroom)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(NetworkTestUtils::waitConnected(client));
	client.connection().setMaxQueuedBytes(80);
	EXPECT_THROW(client.send(makeMessage(16)), spk::Exception);
	const auto transfer = client.send(makeMessage(1));
	EXPECT_EQ(NetworkTestUtils::collect(server.messages(), 1, std::chrono::seconds(5)).size(), 1u);
	EXPECT_TRUE(NetworkTestUtils::waitUntil(std::bind_front(&spk::Transfer::isComplete, transfer), std::chrono::seconds(5)));
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Success);
	client.disconnect();
	server.stop();
}

TEST(TransportLimits, IncomingFrameHeadersEnforceNegotiatedBoundsBeforePayloadAllocation)
{
	using namespace spk::NetworkInternal;
	EXPECT_NO_THROW(IncomingFrameValidator::validate(PageFrameKind, PageMetadataSize + 128, 128, 2048));
	EXPECT_THROW(IncomingFrameValidator::validate(PageFrameKind, PageMetadataSize + 129, 128, 2048), spk::Exception);
	EXPECT_THROW(IncomingFrameValidator::validate(PageFrameKind, PageMetadataSize - 1, 128, 2048), spk::Exception);
	EXPECT_NO_THROW(IncomingFrameValidator::validate(TransferResponse::Codec::FrameKind, TransferResponse::Codec::PayloadSize, 128, 2048));
	EXPECT_THROW(IncomingFrameValidator::validate(TransferResponse::Codec::FrameKind, 4096, 128, 2048), spk::Exception);
	EXPECT_THROW(IncomingFrameValidator::validate(0, 2049, 128, 2048), spk::Exception);
	EXPECT_THROW(IncomingFrameValidator::validate(99, 1, 128, 2048), spk::Exception);
}

TEST(TransportLimits, ServerRejectsOversizedDeclaredPageBeforeReceivingItsBody)
{
	using namespace spk::NetworkInternal;
	spk::Server server;
	server.setMaxPageBytes(128);
	server.start(0);
	auto peer = Socket::connectTCP("127.0.0.1", server.port());
	peer.sendAll(*Handshake::encode(Handshake::Kind::Hello, spk::Connection::InvalidID, spk::Connection::Error::None, 128));
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> reply{};
	ASSERT_TRUE(peer.receiveAllUntil(reply, std::chrono::steady_clock::now() + std::chrono::seconds(3), {}));
	const auto accepted = Handshake::decode(reply);
	ASSERT_TRUE(Handshake::validAcceptance(accepted));
	const auto retained = server.connection(accepted.connectionID);
	ASSERT_TRUE(NetworkTestUtils::waitConnected(retained));

	auto forged = Handshake::encode(Handshake::Kind::Accepted, accepted.connectionID);
	write16(forged->data() + 6, PageFrameKind);
	write32(forged->data() + 28, PageMetadataSize + 129);
	peer.sendAll(std::span<const std::byte>(*forged).first(FrameHeaderSize));
	const auto disconnected = [&] {
		return retained.status() == spk::Connection::Status::Disconnected;
	};
	EXPECT_TRUE(NetworkTestUtils::waitUntil(disconnected, std::chrono::seconds(3)));
	peer.close();
	server.stop();
}

TEST(TransportLimits, ServerRejectsForgedConnectionIDBeforeReceivingFrameBody)
{
	using namespace spk::NetworkInternal;
	spk::Server server;
	server.setMaxPageBytes(128);
	server.start(0);
	auto peer = Socket::connectTCP("127.0.0.1", server.port());
	peer.sendAll(*Handshake::encode(Handshake::Kind::Hello, spk::Connection::InvalidID, spk::Connection::Error::None, 128));
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> reply{};
	ASSERT_TRUE(peer.receiveAllUntil(reply, std::chrono::steady_clock::now() + std::chrono::seconds(3), {}));
	const auto accepted = Handshake::decode(reply);
	ASSERT_TRUE(Handshake::validAcceptance(accepted));
	const auto retained = server.connection(accepted.connectionID);
	ASSERT_TRUE(NetworkTestUtils::waitConnected(retained));

	auto forged = Handshake::encode(Handshake::Kind::Accepted, accepted.connectionID);
	write16(forged->data() + 6, PageFrameKind);
	write64(forged->data() + 20, accepted.connectionID + 1);
	write32(forged->data() + 28, PageMetadataSize + 1);
	peer.sendAll(std::span<const std::byte>(*forged).first(FrameHeaderSize));
	const auto disconnected = [&] {
		return retained.status() == spk::Connection::Status::Disconnected;
	};
	EXPECT_TRUE(NetworkTestUtils::waitUntil(disconnected, std::chrono::seconds(3)));
	peer.close();
	server.stop();
}
