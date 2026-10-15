#include "network/client.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <utility>

using namespace std::chrono_literals;

TEST(ClientSessionIntegrationTest, NewAttemptPreservesSettingsAndRetiresPreviousHandle)
{
	spk::Server server;
	spk::Client client;
	server.setMaxPageBytes(257);
	client.setMaxPageBytes(257);
	client.setMaxMessageBytes(4096);
	client.setHandshakeTimeout(3s);
	server.start(0);

	spk::Connection previous;
	for (std::uint32_t index = 0; index < 3; ++index)
	{
		const auto connection = client.connect("127.0.0.1", server.port());
		ASSERT_TRUE(NetworkTestUtils::waitConnected(connection));
		if (index != 0)
		{
			EXPECT_EQ(previous.status(), spk::Connection::Status::Disconnected);
			EXPECT_NE(previous.id(), connection.id());
		}

		spk::Message::Writer oversized(801);
		oversized.resize(4097);
		EXPECT_THROW((void)client.send(std::move(oversized).build()), spk::Exception);

		spk::Message::Writer writer(802);
		writer.setRequestID(index + 1);
		writer.resize(2048);
		std::fill(writer.data().begin(), writer.data().end(), std::byte{0x5a});
		const auto transfer = client.send(std::move(writer).build());
		auto received = NetworkTestUtils::collect(server.messages(), 1, 5s);
		ASSERT_EQ(received.size(), 1u);
		EXPECT_EQ(received.front().message.requestID(), index + 1);
		EXPECT_EQ(received.front().message.size(), 2048u);
		EXPECT_TRUE(std::all_of(
			received.front().message.data().begin(),
			received.front().message.data().end(),
			[](std::byte value) { return value == std::byte{0x5a}; }));
		ASSERT_TRUE(NetworkTestUtils::waitUntil([&] { return transfer.isComplete(); }, 5s));
		EXPECT_EQ(transfer.status(), spk::Transfer::Status::Success);
		previous = connection;
		client.disconnect();
	}
	server.stop();
}
