#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/client.hpp"

#include <chrono>

TEST(ClientTest, StartsDisconnected)
{
	const spk::Client client;

	EXPECT_FALSE(client.isConnected());
}

TEST(ClientTest, SendingWhileDisconnectedThrows)
{
	spk::Client client;

	EXPECT_THROW(client.send(spk::Message::Writer(1).build()), spk::Exception);
}

TEST(ClientTest, TransferTimeoutConfigurationRejectsNonPositiveValues)
{
	spk::Client client;
	EXPECT_THROW(client.setHandshakeTimeout(std::chrono::milliseconds(0)), spk::Exception);
	EXPECT_THROW(client.setHandshakeTimeout(std::chrono::milliseconds(-1)), spk::Exception);
	EXPECT_NO_THROW(client.setHandshakeTimeout(std::chrono::milliseconds(250)));
	EXPECT_THROW(client.setIncomingTransferTimeout(std::chrono::milliseconds(0)), spk::Exception);
	EXPECT_THROW(client.setAcknowledgmentTimeout(std::chrono::milliseconds(-1)), spk::Exception);
	EXPECT_NO_THROW(client.setIncomingTransferTimeout(std::chrono::milliseconds(250)));
	EXPECT_NO_THROW(client.setAcknowledgmentTimeout(std::chrono::milliseconds(250)));
}

TEST(ClientTest, ConnectRejectsInvalidArgumentsWithoutStartingAttempt)
{
	spk::Client client;
	EXPECT_THROW((void)client.connect("", 1234), spk::Exception);
	EXPECT_THROW((void)client.connect("127.0.0.1", 0), spk::Exception);
	EXPECT_EQ(client.connection().status(), spk::Connection::Status::Disconnected);
	EXPECT_FALSE(client.isConnected());
}
