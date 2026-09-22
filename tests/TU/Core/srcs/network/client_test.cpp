#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/client.hpp"

TEST(ClientTest, StartsDisconnected)
{
	const spk::Client client;

	EXPECT_FALSE(client.isConnected());
}

TEST(ClientTest, SendingWhileDisconnectedThrows)
{
	spk::Client client;

	EXPECT_THROW(client.send(spk::Message(1)), spk::Exception);
}
