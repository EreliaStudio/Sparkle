#include "network/connection.hpp"

#include "exception.hpp"
#include "network/client.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"

#include <atomic>
#include <mutex>
#include <vector>

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
	auto callback = connection.subscribeToStatusChange([](auto, auto) {
		(void)0;
	});
	EXPECT_TRUE(callback.isValid());
}

TEST(Connection, StatusSubscriptionImmediatelyReportsCurrentValue)
{
	spk::Connection connection;
	std::vector<spk::Connection::Status> observed;
	auto contract = connection.subscribeToStatusChange(
		[&](spk::Connection::Status status, spk::Connection::Error) {
			observed.push_back(status);
		});
	ASSERT_EQ(observed.size(), 1u);
	EXPECT_EQ(observed.front(), spk::Connection::Status::Disconnected);
}

TEST(Connection, ClientReconnectDoesNotReanimateOldHandle)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	client.connect("127.0.0.1", server.port());
	spk::Connection previous = client.connection();
	ASSERT_TRUE(previous.isConnected());
	client.disconnect();
	EXPECT_EQ(previous.status(), spk::Connection::Status::Disconnected);
	client.connect("127.0.0.1", server.port());
	EXPECT_FALSE(previous.isConnected());
	EXPECT_TRUE(client.connection().isConnected());
	client.disconnect();
	server.stop();
}

TEST(Connection, ServerRetainedHandleSurvivesStop)
{
	spk::Connection retained;
	{
		spk::Server server;
		spk::Client client;
		std::mutex mutex;
		std::atomic_bool received = false;
		auto contract = server.subscribeToConnectionStatusChange(
			[&](spk::Connection connection, spk::Connection::Status status, spk::Connection::Error) {
				if (status == spk::Connection::Status::Connected)
				{
					const std::scoped_lock lock(mutex);
					retained = connection;
					received = true;
				}
			});
		server.start(0);
		client.connect("127.0.0.1", server.port());
		ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
			return received.load();
		}));
		{
			const std::scoped_lock lock(mutex);
			retained = server.connection(retained.id());
		}
		EXPECT_TRUE(retained.isConnected());
		server.stop();
		EXPECT_EQ(retained.status(), spk::Connection::Status::Disconnected);
		EXPECT_THROW(server.connection(retained.id()), spk::Exception);
		client.disconnect();
	}
	EXPECT_EQ(retained.status(), spk::Connection::Status::Disconnected);
}

TEST(Connection, SetUsesStableSharedHandleIdentity)
{
	spk::Connection first;
	spk::Connection copy = first;
	spk::Connection second;
	const auto originalHash = spk::Connection::Hash{}(first);
	spk::ConnectionSet recipients{first, copy, second};
	EXPECT_EQ(recipients.size(), 2u);
	EXPECT_EQ(recipients.count(copy), 1u);
	EXPECT_EQ(recipients.count(second), 1u);
	EXPECT_EQ(spk::Connection::Hash{}(first), originalHash);
	EXPECT_EQ(first, copy);
	EXPECT_FALSE(first == second);
}

TEST(Connection, SetRetainsIdentityAcrossSessionChanges)
{
	spk::Server server;
	spk::Client client;
	server.start(0);
	client.connect("127.0.0.1", server.port());
	spk::Connection connection = client.connection();
	spk::ConnectionSet recipients{connection};
	EXPECT_EQ(recipients.count(client.connection()), 1u);
	client.disconnect();
	EXPECT_EQ(recipients.count(connection), 1u);
	EXPECT_EQ(recipients.size(), 1u);
	server.stop();
}
