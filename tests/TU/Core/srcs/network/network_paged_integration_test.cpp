#include "network/client.hpp"
#include "network/internal/handshake.hpp"
#include "network/internal/page.hpp"
#include "network/internal/page_scheduler.hpp"
#include "network/internal/socket.hpp"
#include "network/internal/transfer_response.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <stop_token>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	[[nodiscard]] spk::Message makeMessage(std::uint32_t type, std::size_t size)
	{
		spk::Message::Writer writer(type);
		writer.setRequestID(0x123456789ull);
		writer.resize(size);
		for (std::size_t index = 0; index < size; ++index)
		{
			writer.data()[index] = static_cast<std::byte>(index % 251);
		}
		return std::move(writer).build();
	}

	void expectPayload(const spk::Message &message, std::size_t expectedSize)
	{
		ASSERT_EQ(message.size(), expectedSize);
		EXPECT_EQ(message.requestID(), 0x123456789ull);
		for (std::size_t index = 0; index < message.size(); ++index)
		{
			ASSERT_EQ(message.data()[index], static_cast<std::byte>(index % 251)) << index;
		}
	}
}

TEST(NetworkPagedIntegration, LargeLogicalMessageCrossesFormerFrameLimit)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	client.connect("127.0.0.1", server.port());
	EXPECT_TRUE(NetworkTestUtils::waitConnected(client));

	constexpr std::size_t Size = 33u * 1024u * 1024u + 5u;
	client.send(makeMessage(701, Size));
	const auto received = NetworkTestUtils::collect(server.messages(), 1, 20s);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().message.type(), 701u);
	EXPECT_EQ(received.front().message.connectionID(), received.front().emitter);
	expectPayload(received.front().message, Size);

	client.disconnect();
	server.stop();
}

TEST(NetworkPagedIntegration, InterleavedTransfersRestoreExactlyOneMessageEach)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	client.connect("127.0.0.1", server.port());
	EXPECT_TRUE(NetworkTestUtils::waitConnected(client));

	client.send(makeMessage(101, 600000), spk::Message::Priority::Low);
	client.send(makeMessage(102, 17), spk::Message::Priority::High);
	client.send(makeMessage(103, 200000), spk::Message::Priority::Normal);
	auto received = NetworkTestUtils::collect(server.messages(), 3, 10s);
	ASSERT_EQ(received.size(), 3u);
	std::vector<std::uint32_t> types;
	for (const auto &entry : received)
	{
		types.push_back(entry.message.type());
		if (entry.message.type() == 101)
		{
			expectPayload(entry.message, 600000);
		}
		if (entry.message.type() == 102)
		{
			expectPayload(entry.message, 17);
		}
		if (entry.message.type() == 103)
		{
			expectPayload(entry.message, 200000);
		}
	}
	std::sort(types.begin(), types.end());
	EXPECT_EQ(types, (std::vector<std::uint32_t>{101, 102, 103}));

	client.disconnect();
	server.stop();
}

TEST(NetworkPagedIntegration, ServerSendsPagedResponseWithoutSynchronousSocketWrites)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	client.connect("127.0.0.1", server.port());
	EXPECT_TRUE(NetworkTestUtils::waitConnected(client));
	client.send(makeMessage(201, 1));
	const auto requests = NetworkTestUtils::collect(server.messages(), 1, 5s);
	ASSERT_EQ(requests.size(), 1u);

	server.sendTo(server.connection(requests.front().emitter), makeMessage(202, 1200000), spk::Message::Priority::High);
	const auto responses = NetworkTestUtils::collect(client.messages(), 1, 10s);
	ASSERT_EQ(responses.size(), 1u);
	EXPECT_EQ(responses.front().type(), 202u);
	EXPECT_EQ(responses.front().connectionID(), requests.front().emitter);
	expectPayload(responses.front(), 1200000);

	client.disconnect();
	server.stop();
}

TEST(NetworkPagedIntegration, TransferSuccessRequiresReceiverAcknowledgment)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	client.connect("127.0.0.1", server.port());
	EXPECT_TRUE(NetworkTestUtils::waitConnected(client));

	auto transfer = client.send(makeMessage(801, 200000));
	EXPECT_NE(transfer.id(), spk::Transfer::InvalidID);
	std::promise<spk::Transfer::Status> promise;
	auto completed = promise.get_future();
	auto contract = transfer.subscribeToCompletion([&](spk::Transfer::Status status, spk::Transfer::Reason reason) {
		EXPECT_EQ(reason, spk::Transfer::Reason::None);
		promise.set_value(status);
	});

	const auto received = NetworkTestUtils::collect(server.messages(), 1, 10s);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().message.type(), 801u);
	ASSERT_EQ(completed.wait_for(10s), std::future_status::ready);
	EXPECT_EQ(completed.get(), spk::Transfer::Status::Success);
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Success);
	EXPECT_TRUE(client.isConnected());

	client.disconnect();
	server.stop();
}

TEST(NetworkPagedIntegration, IncompleteTransferExpiresWithoutDisconnectingServerSession)
{
	using namespace spk::NetworkInternal;
	spk::Server server;
	server.setIncomingTransferTimeout(100ms);
	server.start(0);
	auto socket = Socket::connectTCP("127.0.0.1", server.port());
	socket.sendAll(*Handshake::encode(Handshake::Kind::Hello));
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> helloResponse{};
	ASSERT_TRUE(socket.receiveAllUntil(helloResponse, std::chrono::steady_clock::now() + 3s, {}));
	const auto accepted = Handshake::decode(helloResponse);
	ASSERT_TRUE(Handshake::validAcceptance(accepted));

	spk::Message::Writer writer(900);
	writer.setConnectionID(accepted.connectionID);
	writer.resize(12);
	PageScheduler scheduler;
	(void)scheduler.enqueue(std::move(writer).build());
	const auto first = scheduler.nextPage(4);
	ASSERT_TRUE(first.has_value());
	socket.sendAll(*encodePage(*first));
	std::array<std::byte, FrameHeaderSize + TransferResponse::PayloadSize> reply{};
	ASSERT_TRUE(socket.receiveAllUntil(reply, std::chrono::steady_clock::now() + 4s, {}));
	const auto expired = TransferResponse::decode(reply);
	EXPECT_EQ(expired.status, spk::Transfer::Status::Failed);
	EXPECT_EQ(expired.reason, spk::Transfer::Reason::InactivityTimeout);
	EXPECT_EQ(expired.transferID, first->transferID);
	EXPECT_EQ(expired.connectionID, accepted.connectionID);

	spk::Message::Writer healthy(901);
	healthy.setConnectionID(accepted.connectionID);
	healthy.resize(3);
	(void)scheduler.enqueue(std::move(healthy).build());
	while (const auto page = scheduler.nextPage(4))
	{
		if (page->transferID != first->transferID)
		{
			socket.sendAll(*encodePage(*page));
		}
	}
	const auto received = NetworkTestUtils::collect(server.messages(), 1, 3s);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().message.type(), 901u);
	EXPECT_EQ(received.front().emitter, accepted.connectionID);
	socket.close();
	server.stop();
}
