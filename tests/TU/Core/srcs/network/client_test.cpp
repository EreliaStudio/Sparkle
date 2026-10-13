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
	EXPECT_THROW(client.setIncomingTransferTimeout(std::chrono::milliseconds(0)), spk::Exception);
	EXPECT_THROW(client.setAcknowledgmentTimeout(std::chrono::milliseconds(-1)), spk::Exception);
	EXPECT_NO_THROW(client.setIncomingTransferTimeout(std::chrono::milliseconds(250)));
	EXPECT_NO_THROW(client.setAcknowledgmentTimeout(std::chrono::milliseconds(250)));
}
