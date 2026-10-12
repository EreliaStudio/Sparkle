#include "network/client.hpp"
#include "network/server.hpp"

#include <gtest/gtest.h>

#include <chrono>
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
		auto subscription = server.subscribeToConnection([&](spk::ConnectionID id) {
			{
				std::scoped_lock lock(mutex);
				assigned = id;
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
