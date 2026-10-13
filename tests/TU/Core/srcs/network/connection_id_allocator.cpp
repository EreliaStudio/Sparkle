#include "network/client.hpp"
#include "network/server.hpp"

#include "exception.hpp"
#include "network/internal/connection_id.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <limits>
#include <condition_variable>
#include <mutex>

namespace
{
	spk::ConnectionID connectAndReadID()
	{
		spk::Server server;
		std::mutex mutex;
		std::condition_variable changed;
		spk::ConnectionID assigned = spk::InvalidConnectionID;
		auto subscription = server.subscribeToConnectionStatusChange(
			[&](spk::Connection connection, spk::Connection::Status status, spk::Connection::Error) {
				if (status != spk::Connection::Status::Connected)
				{
					return;
				}
				{
					std::scoped_lock lock(mutex);
					assigned = connection.id();
				}
				changed.notify_all();
			});
		server.start(0);

		spk::Client client;
		client.connect("127.0.0.1", server.port());
		{
			std::unique_lock lock(mutex);
			EXPECT_TRUE(changed.wait_for(lock, std::chrono::seconds(3), [&] {
				return assigned != spk::InvalidConnectionID;
			}));
		}
		client.disconnect();
		server.stop();
		return assigned;
	}
}

TEST(NetworkConnectionID, NewServerDoesNotReuseOldConnectionID)
{
	const auto first = connectAndReadID();
	const auto second = connectAndReadID();

	EXPECT_NE(first, spk::InvalidConnectionID);
	EXPECT_GT(second, first);
}

TEST(NetworkConnectionID, TokenMappingNeverWrapsIntoReservedEpollValues)
{
	using ID = spk::Connection::ID;
	std::atomic<ID> next{spk::NetworkInternal::MaximumConnectionID};
	const ID last = spk::NetworkInternal::allocateConnectionID(next);
	EXPECT_EQ(last, std::numeric_limits<ID>::max() - spk::NetworkInternal::ConnectionTokenOffset);
	EXPECT_EQ(last + spk::NetworkInternal::ConnectionTokenOffset, std::numeric_limits<ID>::max());
	EXPECT_THROW((void)spk::NetworkInternal::allocateConnectionID(next), spk::Exception);
	std::atomic<ID> invalid{spk::Connection::InvalidID};
	EXPECT_THROW((void)spk::NetworkInternal::allocateConnectionID(invalid), spk::Exception);
}
