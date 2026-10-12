#include "network/connection.hpp"

#include <gtest/gtest.h>

TEST(Connection, CopiedHandlesShareConfiguration)
{
	spk::Connection connection;
	spk::Connection copy = connection;
	connection.setMaxQueuedBytes(1234);
	EXPECT_EQ(copy.maxQueuedBytes(), 1234u);
	copy.setMaxQueuedBytes(5678);
	EXPECT_EQ(connection.maxQueuedBytes(), 5678u);
}

TEST(Connection, NewConnectionStartsDisconnected)
{
	spk::Connection connection;
	EXPECT_EQ(connection.id(), spk::Connection::InvalidID);
	EXPECT_EQ(connection.status(), spk::Connection::Status::Disconnected);
	EXPECT_EQ(connection.error(), spk::Connection::Error::None);
	EXPECT_FALSE(connection.isConnected());
}

TEST(Connection, CanSubscribeToStatusChanges)
{
	spk::Connection connection;
	auto callback = connection.subscribeToStatusChange([](
		spk::Connection::Status, spk::Connection::Error) {});
	EXPECT_TRUE(callback.isValid());
}
