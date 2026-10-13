#include "network/client.hpp"
#include "network/internal/handshake.hpp"
#include "network/internal/page.hpp"
#include "network/internal/page_reassembler.hpp"
#include "network/internal/page_scheduler.hpp"
#include "network/internal/reassembly_budget.hpp"
#include "network/internal/socket.hpp"
#include "network/internal/transfer_response.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <stop_token>
#include <thread>
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

TEST(NetworkPagedIntegration, ConflictingPageFailsOnlyItsTransfer)
{
	using namespace spk::NetworkInternal;
	spk::Server server;
	server.start(0);
	auto socket = Socket::connectTCP("127.0.0.1", server.port());
	socket.sendAll(*Handshake::encode(Handshake::Kind::Hello));
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> acceptance{};
	ASSERT_TRUE(socket.receiveAllUntil(acceptance, std::chrono::steady_clock::now() + 3s, {}));
	const auto accepted = Handshake::decode(acceptance);
	ASSERT_TRUE(Handshake::validAcceptance(accepted));

	spk::Message::Writer broken(910);
	broken.setConnectionID(accepted.connectionID);
	broken.resize(12);
	PageScheduler scheduler;
	(void)scheduler.enqueue(std::move(broken).build());
	const auto first = scheduler.nextPage(4);
	ASSERT_TRUE(first.has_value());
	auto initial = encodePage(*first);
	socket.sendAll(*initial);
	auto conflicting = *initial;
	conflicting[FrameHeaderSize + PageMetadataSize] = std::byte{0xEF};
	socket.sendAll(conflicting);

	std::array<std::byte, FrameHeaderSize + TransferResponse::PayloadSize> reply{};
	ASSERT_TRUE(socket.receiveAllUntil(reply, std::chrono::steady_clock::now() + 3s, {}));
	const auto failed = TransferResponse::decode(reply);
	EXPECT_EQ(failed.transferID, first->transferID);
	EXPECT_EQ(failed.status, spk::Transfer::Status::Failed);
	EXPECT_EQ(failed.reason, spk::Transfer::Reason::ConflictingPage);
	EXPECT_TRUE(server.isRunning());

	spk::Message::Writer healthy(911);
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
	EXPECT_EQ(received.front().message.type(), 911u);
	EXPECT_EQ(received.front().emitter, accepted.connectionID);
	socket.close();
	server.stop();
}

TEST(NetworkPagedIntegration, ServerGlobalIncomingLimitRejectsTransferWithoutClosingPeer)
{
	using namespace spk::NetworkInternal;
	spk::Server server;
	server.setGlobalMaxIncomingBytes(1);
	server.start(0);
	auto socket = Socket::connectTCP("127.0.0.1", server.port());
	socket.sendAll(*Handshake::encode(Handshake::Kind::Hello));
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> responseBytes{};
	ASSERT_TRUE(socket.receiveAllUntil(responseBytes, std::chrono::steady_clock::now() + 3s, {}));
	const auto accepted = Handshake::decode(responseBytes);
	ASSERT_TRUE(Handshake::validAcceptance(accepted));

	spk::Message::Writer oversized(930);
	oversized.setConnectionID(accepted.connectionID);
	oversized.resize(16);
	const auto rejectedMessage = std::move(oversized).build();
	socket.sendAll(*encodePage(Page{rejectedMessage, 1, rejectedMessage.size(), 0, 4}));
	std::array<std::byte, FrameHeaderSize + TransferResponse::PayloadSize> rejectionBytes{};
	ASSERT_TRUE(socket.receiveAllUntil(rejectionBytes, std::chrono::steady_clock::now() + 3s, {}));
	const auto rejected = TransferResponse::decode(rejectionBytes);
	EXPECT_EQ(rejected.status, spk::Transfer::Status::Rejected);
	EXPECT_EQ(rejected.reason, spk::Transfer::Reason::ReceiverCapacity);
	EXPECT_EQ(rejected.transferID, 1u);
	socket.sendAll(*encodePage(Page{rejectedMessage, 1, rejectedMessage.size(), 4, 4}));
	EXPECT_TRUE(server.connection(accepted.connectionID).isConnected());

	server.setGlobalMaxIncomingBytes(4096);
	spk::Message::Writer healthy(931);
	healthy.setConnectionID(accepted.connectionID);
	healthy.resize(3);
	const auto healthyMessage = std::move(healthy).build();
	socket.sendAll(*encodePage(Page{healthyMessage, 2, healthyMessage.size(), 0, healthyMessage.size()}));
	const auto received = NetworkTestUtils::collect(server.messages(), 1, 3s);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().message.type(), 931u);
	EXPECT_EQ(received.front().emitter, accepted.connectionID);
	socket.close();
	server.stop();
}

TEST(NetworkPagedIntegration, CompletedTransferReplayIsNotDispatchedTwice)
{
	using namespace spk::NetworkInternal;
	spk::Server server;
	server.start(0);
	auto socket = Socket::connectTCP("127.0.0.1", server.port());
	socket.sendAll(*Handshake::encode(Handshake::Kind::Hello));
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> acceptance{};
	ASSERT_TRUE(socket.receiveAllUntil(acceptance, std::chrono::steady_clock::now() + 3s, {}));
	const auto accepted = Handshake::decode(acceptance);
	ASSERT_TRUE(Handshake::validAcceptance(accepted));

	spk::Message::Writer writer(940);
	writer.setConnectionID(accepted.connectionID);
	writer.resize(3);
	const auto message = std::move(writer).build();
	const auto frame = encodePage(Page{message, 7, message.size(), 0, message.size()});
	socket.sendAll(*frame);
	std::array<std::byte, FrameHeaderSize + TransferResponse::PayloadSize> responseBytes{};
	ASSERT_TRUE(socket.receiveAllUntil(responseBytes, std::chrono::steady_clock::now() + 3s, {}));
	EXPECT_EQ(TransferResponse::decode(responseBytes).status, spk::Transfer::Status::Success);

	socket.sendAll(*frame);
	const auto received = NetworkTestUtils::collect(server.messages(), 2, 200ms);
	ASSERT_EQ(received.size(), 1u);
	EXPECT_EQ(received.front().message.type(), 940u);
	EXPECT_TRUE(server.connection(accepted.connectionID).isConnected());

	spk::Message::Writer next(941);
	next.setConnectionID(accepted.connectionID);
	next.resize(3);
	const auto healthy = std::move(next).build();
	socket.sendAll(*encodePage(Page{healthy, 8, healthy.size(), 0, healthy.size()}));
	const auto later = NetworkTestUtils::collect(server.messages(), 1, 3s);
	ASSERT_EQ(later.size(), 1u);
	EXPECT_EQ(later.front().message.type(), 941u);
	socket.close();
	server.stop();
}

TEST(NetworkPagedIntegration, LateAcknowledgmentCannotChangeTimedOutTransfer)
{
	using namespace spk::NetworkInternal;
	auto listener = Socket::listenTCP(0);
	spk::Client client;
	client.setAcknowledgmentTimeout(100ms);
	const auto connection = client.connect("127.0.0.1", listener.localPort());
	auto peer = listener.accept();
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> hello{};
	ASSERT_TRUE(peer.receiveAllUntil(hello, std::chrono::steady_clock::now() + 3s, {}));
	peer.sendAll(*Handshake::encode(Handshake::Kind::Accepted, 987));
	ASSERT_TRUE(NetworkTestUtils::waitConnected(connection));

	auto transfer = client.send(makeMessage(962, 8));
	std::atomic_size_t callbacks = 0;
	auto contract = transfer.subscribeToCompletion([&](spk::Transfer::Status, spk::Transfer::Reason) {
		++callbacks;
	});
	ASSERT_TRUE(NetworkTestUtils::waitUntil(std::bind_front(&spk::Transfer::isComplete, transfer), 4s));
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Indeterminate);
	EXPECT_EQ(transfer.reason(), spk::Transfer::Reason::AcknowledgmentTimeout);
	EXPECT_EQ(callbacks.load(), 1u);

	const TransferResponse late{987, transfer.id(), spk::Transfer::Status::Success, spk::Transfer::Reason::None};
	peer.sendAll(*late.encode());
	std::this_thread::sleep_for(100ms);
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Indeterminate);
	EXPECT_EQ(callbacks.load(), 1u);
	EXPECT_TRUE(client.isConnected());
	client.disconnect();
	peer.close();
}

TEST(NetworkPagedIntegration, GlobalIncomingBudgetIsSharedAcrossSimultaneousPeers)
{
	using namespace spk::NetworkInternal;
	auto probeBudget = std::make_shared<ReassemblyBudget>(4096);
	PageReassembler probe(1024, 4096, 60s, probeBudget);
	const auto prototype = makeMessage(970, 128);
	const auto probeFrame = encodePage(Page{prototype, 1, 128, 0, 16});
	EXPECT_FALSE(probe.accept(decodePage(*probeFrame)).has_value());
	const auto charge = probeBudget->used();
	ASSERT_GT(charge, 128u);
	probe.clear();

	spk::Server server;
	server.setGlobalMaxIncomingBytes(charge * 2 - 1);
	server.start(0);
	auto connectRaw = [&server] {
		auto socket = Socket::connectTCP("127.0.0.1", server.port());
		socket.sendAll(*Handshake::encode(Handshake::Kind::Hello));
		std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> hello{};
		if (!socket.receiveAllUntil(hello, std::chrono::steady_clock::now() + 3s, {}))
		{
			throw spk::Exception("Unable to receive raw peer acceptance.");
		}
		return std::make_pair(std::move(socket), Handshake::decode(hello));
	};
	auto [firstSocket, firstAcceptance] = connectRaw();
	auto [secondSocket, secondAcceptance] = connectRaw();
	ASSERT_TRUE(Handshake::validAcceptance(firstAcceptance));
	ASSERT_TRUE(Handshake::validAcceptance(secondAcceptance));

	const auto first = prototype.withConnectionID(firstAcceptance.connectionID);
	const auto second = prototype.withConnectionID(secondAcceptance.connectionID);
	firstSocket.sendAll(*encodePage(Page{first, 1, first.size(), 0, 16}));
	std::this_thread::sleep_for(100ms);
	secondSocket.sendAll(*encodePage(Page{second, 1, second.size(), 0, 16}));
	std::array<std::byte, FrameHeaderSize + TransferResponse::PayloadSize> responseBytes{};
	ASSERT_TRUE(secondSocket.receiveAllUntil(responseBytes, std::chrono::steady_clock::now() + 3s, {}));
	const auto rejected = TransferResponse::decode(responseBytes);
	EXPECT_EQ(rejected.status, spk::Transfer::Status::Rejected);
	EXPECT_EQ(rejected.reason, spk::Transfer::Reason::ReceiverCapacity);
	EXPECT_TRUE(server.connection(secondAcceptance.connectionID).isConnected());

	firstSocket.sendAll(*encodePage(Page{first, 1, first.size(), 16, first.size() - 16}));
	const auto receivedFirst = NetworkTestUtils::collect(server.messages(), 1, 3s);
	ASSERT_EQ(receivedFirst.size(), 1u);
	EXPECT_EQ(receivedFirst.front().emitter, firstAcceptance.connectionID);

	secondSocket.sendAll(*encodePage(Page{second, 2, second.size(), 0, second.size()}));
	const auto receivedSecond = NetworkTestUtils::collect(server.messages(), 1, 3s);
	ASSERT_EQ(receivedSecond.size(), 1u);
	EXPECT_EQ(receivedSecond.front().emitter, secondAcceptance.connectionID);
	firstSocket.close();
	secondSocket.close();
	server.stop();
}

TEST(NetworkPagedIntegration, DisconnectInterruptsBlockedClientPayloadWrite)
{
	using namespace spk::NetworkInternal;
	auto listener = Socket::listenTCP(0);
	spk::Client client;
	const auto connection = client.connect("127.0.0.1", listener.localPort());
	auto peer = listener.accept();
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> hello{};
	ASSERT_TRUE(peer.receiveAllUntil(hello, std::chrono::steady_clock::now() + 3s, {}));
	peer.sendAll(*Handshake::encode(Handshake::Kind::Accepted, 988));
	ASSERT_TRUE(NetworkTestUtils::waitConnected(connection));
	const auto transfer = client.send(makeMessage(980, 8u * 1024u * 1024u));
	std::this_thread::sleep_for(100ms);
	auto disconnected = std::async(std::launch::async, [&client] {
		client.disconnect();
	});
	ASSERT_EQ(disconnected.wait_for(5s), std::future_status::ready);
	EXPECT_NO_THROW(disconnected.get());
	EXPECT_EQ(connection.status(), spk::Connection::Status::Disconnected);
	EXPECT_TRUE(transfer.isComplete());
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Indeterminate);
	peer.close();
}

TEST(NetworkPagedIntegration, ReplayOlderThanOneThousandCompletedTransfersNeverRepublishes)
{
	using namespace spk::NetworkInternal;
	spk::Server server;
	server.start(0);
	auto socket = Socket::connectTCP("127.0.0.1", server.port());
	socket.sendAll(*Handshake::encode(Handshake::Kind::Hello));
	std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> acceptance{};
	ASSERT_TRUE(socket.receiveAllUntil(acceptance, std::chrono::steady_clock::now() + 3s, {}));
	const auto accepted = Handshake::decode(acceptance);
	ASSERT_TRUE(Handshake::validAcceptance(accepted));

	spk::Message::Writer writer(975);
	writer.setConnectionID(accepted.connectionID);
	writer.resize(1);
	const auto message = std::move(writer).build();
	auto frame = encodePage(Page{message, 1, message.size(), 0, message.size()});
	std::array<std::byte, FrameHeaderSize + TransferResponse::PayloadSize> answer{};
	for (std::uint64_t id = 1; id <= 1200; ++id)
	{
		write64(frame->data() + FrameHeaderSize, id);
		socket.sendAll(*frame);
		ASSERT_TRUE(socket.receiveAllUntil(answer, std::chrono::steady_clock::now() + 5s, {}));
		const auto response = TransferResponse::decode(answer);
		ASSERT_EQ(response.transferID, id);
		ASSERT_EQ(response.status, spk::Transfer::Status::Success);
	}
	write64(frame->data() + FrameHeaderSize, 1);
	socket.sendAll(*frame);
	const auto messages = NetworkTestUtils::collect(server.messages(), 1201, 250ms);
	EXPECT_EQ(messages.size(), 1200u);
	EXPECT_TRUE(server.connection(accepted.connectionID).isConnected());
	socket.close();
	server.stop();
}
