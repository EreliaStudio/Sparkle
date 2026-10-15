#include "network/client.hpp"
#include "network/internal/socket.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

using namespace std::chrono_literals;

TEST(NetworkMemorySafetyTest, RepeatedPagedTransferSessionsReleaseResources)
{
	spk::Server server;
	server.setMaxPageBytes(2048);
	server.start(0);
	for (std::uint32_t index = 0; index < 12; ++index)
	{
		auto client = std::make_unique<spk::Client>();
		client->setMaxPageBytes(2048);
		const auto session = client->connect("127.0.0.1", server.port());
		ASSERT_TRUE(NetworkTestUtils::waitConnected(session));
		spk::Message::Writer writer(901);
		writer.setRequestID(index + 1);
		writer.resize(65536);
		const auto transfer = client->send(std::move(writer).build());
		const auto received = NetworkTestUtils::collect(server.messages(), 1, 10s);
		ASSERT_EQ(received.size(), 1u);
		EXPECT_EQ(received.front().message.size(), 65536u);
		EXPECT_EQ(received.front().message.requestID(), index + 1);
		const auto completed = [&] {
			return transfer.isComplete();
		};
		EXPECT_TRUE(NetworkTestUtils::waitUntil(completed, 5s));
		EXPECT_EQ(transfer.status(), spk::Transfer::Status::Success);
	}
	server.stop();
}

TEST(NetworkMemorySafetyTest, PendingHandshakeCancellationReleasesResources)
{
	auto listener = spk::NetworkInternal::Socket::listenTCP(0);
	for (std::size_t index = 0; index < 16; ++index)
	{
		spk::Client client;
		const auto session = client.connect("127.0.0.1", listener.localPort());
		client.disconnect();
		EXPECT_EQ(session.status(), spk::Connection::Status::Disconnected);
	}
	listener.close();
}
