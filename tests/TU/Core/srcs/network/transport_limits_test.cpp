#include "exception.hpp"
#include "network/client.hpp"
#include "network/internal/page_scheduler.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <gtest/gtest.h>

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
