#include "network_replication/codec.hpp"
#include "network_replication/id.hpp"
#include "network_replication/publisher.hpp"
#include <gtest/gtest.h>
#include <network/replication/protocol.hpp>
#include <network/replication/receiver.hpp>
#include <network/replication/request_queue.hpp>
#include <network/replication/request_service.hpp>
using namespace spk::Network;
using namespace std::chrono_literals;
using namespace ReplicationTest;
TEST(NetworkReplicationTest, All65ContinuouslyDirtyObjectsArePublished)
{
	auto server = publisher();
	(void)server.open(id(200));
	std::map<ObjectID, int> observed, lastSeen;
	for (unsigned i = 1; i <= 65; ++i)
	{
		server.publish(id(i), {0});
		server.follow(id(200), id(i));
	}
	for (int tick = 0; tick < 100; ++tick)
	{
		for (unsigned i = 1; i <= 65; ++i)
		{
			server.publish(id(i), {tick});
		}
		server.dispatch(Clock::time_point{}, 64, [&](auto, const auto &update) {
			++observed[update.object];
			lastSeen[update.object] = tick;
			return true;
		});
		if (tick > 0)
		{
			ASSERT_EQ(lastSeen.size(), 65u);
			for (const auto &[key, seen] : lastSeen)
			{
				EXPECT_GE(seen, tick - 1);
			}
		}
	}
	ASSERT_EQ(observed.size(), 65);
	for (const auto &[key, count] : observed)
	{
		EXPECT_GE(count, 50);
	}
}

TEST(NetworkReplicationTest, CoalescesLatestStateWithoutMovingItsQueuePosition)
{
	auto server = publisher();
	(void)server.open(id(200));
	for (unsigned i = 1; i <= 3; ++i)
	{
		server.publish(id(i), {0});
		server.follow(id(200), id(i));
	}
	server.publish(id(1), {7});
	std::vector<ObjectID> order;
	server.dispatch({}, 3, [&](auto, const auto &update) {
		order.push_back(update.object);
		if (update.object == id(1))
		{
			EXPECT_EQ(update.state->value, 7);
		}
		return true;
	});
	EXPECT_EQ(order, (std::vector<ObjectID>{id(1), id(2), id(3)}));
}

TEST(NetworkReplicationTest, BlockedPeerDoesNotBlockHealthyPeer)
{
	auto server = publisher();
	(void)server.open(id(1));
	(void)server.open(id(2));
	server.publish(id(3), {4});
	server.follow(id(1), id(3));
	server.follow(id(2), id(3));
	const auto result =
		server.dispatch({}, 4, [](auto peer, const auto &) {
			return peer != id(1);
		});
	EXPECT_EQ(result.sent, 1);
	EXPECT_EQ(result.blocked, 1);
	const auto recovery = server.dispatch({}, 4, [](auto, const auto &) {
		return true;
	});
	EXPECT_EQ(recovery.sent, 1);
}

TEST(NetworkReplicationTest, ThrowingSenderDoesNotBlockHealthyPeer)
{
	auto server = publisher();
	(void)server.open(id(1));
	(void)server.open(id(2));
	server.publish(id(3), {4});
	server.follow(id(1), id(3));
	server.follow(id(2), id(3));
	auto result = server.dispatch({}, 4, [](auto peer, const auto &) {
		if (peer == id(1))
		{
			throw spk::Exception("transport unavailable");
		}
		return true;
	});
	EXPECT_EQ(result.sent, 1);
	EXPECT_EQ(result.errors, 1);
}

TEST(NetworkReplicationTest, TerminationCannotBeUndoneByLateState)
{
	Receiver<State> client;
	client.reset(id(1));
	Update<State> update{id(1), id(2), 1, 1, Edit::Set, std::make_shared<State>(State{3})};
	ASSERT_TRUE(client.receive(update));
	auto stop = update;
	stop.edit = Edit::Forget;
	stop.state.reset();
	ASSERT_TRUE(client.receive(stop));
	update.revision = 100;
	EXPECT_FALSE(client.receive(update));
	update.tracking = 2;
	EXPECT_TRUE(client.receive(update));
	EXPECT_FALSE(client.receive(stop));
}

TEST(NetworkReplicationTest, SessionResetRejectsOldTraffic)
{
	Receiver<State> client;
	client.reset(id(1));
	client.reset(id(2));
	EXPECT_FALSE(client.receive(
		{id(1), id(3), 1, 1, Edit::Set, std::make_shared<State>(State{4})}));
	EXPECT_TRUE(client.receive(
		{id(2), id(3), 1, 1, Edit::Set, std::make_shared<State>(State{4})}));
}

TEST(NetworkReplicationTest, PublisherReintroductionUsesNewTracking)
{
	auto server = publisher();
	(void)server.open(id(1));
	server.publish(id(2), {0});
	server.follow(id(1), id(2));
	std::vector<Update<State>> updates;
	auto send = [&](auto, const auto &update) {
		updates.push_back(update);
		return true;
	};
	server.dispatch({}, 10, send);
	server.forget(id(1), id(2));
	server.dispatch({}, 10, send);
	server.follow(id(1), id(2));
	server.dispatch({}, 10, send);
	server.destroy(id(2));
	server.dispatch({}, 10, send);
	ASSERT_EQ(updates.size(), 4);
	EXPECT_EQ(updates[1].edit, Edit::Forget);
	EXPECT_GT(updates[2].tracking, updates[0].tracking);
	EXPECT_EQ(updates[3].edit, Edit::Destroy);
}

TEST(NetworkReplicationTest, PublicationIntervalCoalescesChanges)
{
	Publisher<State> server({.interval = 50ms});
	(void)server.open(id(1));
	server.publish(id(2), {0});
	server.follow(id(1), id(2));
	auto send = [](auto, const auto &) {
		return true;
	};
	EXPECT_EQ(server.dispatch({}, 5, send).sent, 1);
	server.publish(id(2), {1});
	EXPECT_EQ(server.dispatch(Clock::time_point{} + 49ms, 5, send).sent, 0);
	EXPECT_EQ(server.dispatch(Clock::time_point{} + 50ms, 5, send).sent, 1);
}

TEST(NetworkReplicationTest, LimitsRejectExplicitlyWithoutSilentEviction)
{
	Publisher<State> server({.maximumObjects = 1, .maximumPeers = 1, .interval = 0ms});
	const auto session = server.open(id(1));
	EXPECT_THROW((void)server.open(id(2)), spk::Exception);
	server.publish(id(2), {1});
	server.follow(id(1), id(2));
	server.destroy(id(2));
	server.publish(id(3), {1});
	EXPECT_THROW(server.follow(id(1), id(3)), spk::Exception);
	server.dispatch({}, 10, [](auto, const auto &) {
		return true;
	});
	EXPECT_NO_THROW(server.follow(id(1), id(3)));
	Receiver<State> receiver(1);
	receiver.reset(session);
	EXPECT_TRUE(receiver.receive({session, id(2), 1, 1, Edit::Destroy, nullptr}));
	EXPECT_THROW((void)receiver.receive({session, id(3), 2, 1, Edit::Destroy, nullptr}), spk::Exception);
}
