#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/server.hpp"

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

TEST(ServerTest, SendingWhileStoppedThrows)
{
	spk::Server server;

	EXPECT_THROW(server.sendToAll(spk::Message(1)), spk::Exception);
}
