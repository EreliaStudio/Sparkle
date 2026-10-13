#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/server.hpp"

#include <chrono>

TEST(ServerTest, StartsOnRequestedEphemeralPortAndStops)
{
	spk::Server server;

	server.start(0);

	EXPECT_TRUE(server.isRunning());
	EXPECT_NE(server.port(), 0u);

	server.stop();
	EXPECT_FALSE(server.isRunning());
	EXPECT_EQ(server.port(), 0u);
}

TEST(ServerTest, StartingRunningServerThrowsWithoutReplacingListener)
{
	spk::Server server;
	server.start(0);
	const auto originalPort = server.port();
	EXPECT_THROW(server.start(0), spk::Exception);
	EXPECT_TRUE(server.isRunning());
	EXPECT_EQ(server.port(), originalPort);
	server.stop();
}

TEST(ServerTest, BroadcastWithoutRecipientsThrows)
{
	spk::Server server;
	server.start(0);
	EXPECT_THROW(server.sendToAll(spk::Message::Writer(1).build()), spk::Exception);
	EXPECT_THROW(server.sendTo(spk::ConnectionSet{}, spk::Message::Writer(1).build()), spk::Exception);
	server.stop();
}

TEST(ServerTest, SendingWhileStoppedThrows)
{
	spk::Server server;

	EXPECT_THROW(server.sendToAll(spk::Message::Writer(1).build()), spk::Exception);
}

TEST(ServerTest, RejectsInvalidConnectionHandle)
{
	spk::Server server;
	server.start(0);
	EXPECT_THROW(server.sendTo(spk::Connection(), spk::Message::Writer(1).build()), spk::Exception);
	server.stop();
}

TEST(ServerTest, TransferTimeoutConfigurationRejectsNonPositiveValues)
{
	spk::Server server;
	EXPECT_THROW(server.setIncomingTransferTimeout(std::chrono::milliseconds(0)), spk::Exception);
	EXPECT_THROW(server.setAcknowledgmentTimeout(std::chrono::milliseconds(-1)), spk::Exception);
	EXPECT_NO_THROW(server.setIncomingTransferTimeout(std::chrono::milliseconds(250)));
	EXPECT_NO_THROW(server.setAcknowledgmentTimeout(std::chrono::milliseconds(250)));
	server.start(0);
	EXPECT_NO_THROW(server.setIncomingTransferTimeout(std::chrono::milliseconds(500)));
	EXPECT_NO_THROW(server.setAcknowledgmentTimeout(std::chrono::milliseconds(500)));
	server.stop();
}
