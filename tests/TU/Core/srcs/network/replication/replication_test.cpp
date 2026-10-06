#include "network/network_traits_test.hpp"
using namespace std::chrono_literals;

TEST_F(NetworkTraitsTest, All65ContinuouslyDirtyObjectsArePublishedWithoutStarvation)
{
	Source source({.interval = 0ms});
	(void)source.openPeer(peer);
	std::map<ID, Object> objects;
	for (int i = 0; i < 65; ++i)
	{
		const auto id = ID::generate();
		source.registerObject(id, objects[id]);
		source.follow(peer, id);
	}
	std::map<ID, int> observed, lastSeen;
	for (int tick = 0; tick < 100; ++tick)
	{
		for (auto &[id, entity] : objects)
		{
			entity.change(tick);
		}
		source.sent.clear();
		(void)source.dispatch(now, 64);
		for (const auto &[recipient, message] : source.sent)
		{
			const auto state = protocol.decodeUpdate(message);
			++observed[state.object];
			lastSeen[state.object] = tick;
			EXPECT_EQ(state.state->value, tick);
		}
		if (tick > 0)
		{
			ASSERT_EQ(lastSeen.size(), 65u);
			for (const auto &[id, seen] : lastSeen)
			{
				EXPECT_GE(seen, tick - 1);
			}
		}
	}
	for (const auto &[id, count] : observed)
	{
		EXPECT_GE(count, 50);
	}
}

TEST_F(NetworkTraitsTest, CoalescingRetainsFifoPosition)
{
	Source source;
	(void)source.openPeer(peer);
	std::map<ID, Object> objects;
	std::vector<ID> expected;
	for (int i = 0; i < 3; ++i)
	{
		const auto id = ID::generate();
		expected.push_back(id);
		source.registerObject(id, objects[id]);
		source.follow(peer, id);
	}
	objects.at(expected.front()).change(7);
	EXPECT_EQ(source.dispatch(now, 3).sent, 3u);
	std::vector<ID> actual;
	for (const auto &[recipient, message] : source.sent)
	{
		actual.push_back(protocol.decodeUpdate(message).object);
	}
	EXPECT_EQ(actual, expected);
	EXPECT_EQ(protocol.decodeUpdate(source.sent.front().second).state->value, 7);
}

TEST_F(NetworkTraitsTest, ReintroductionAfterForgetOrDestructionUsesNewTracking)
{
	Source source({.interval = 0ms});
	Object entity;
	session = source.openPeer(peer);
	source.registerObject(object, entity);
	source.follow(peer, object);
	(void)source.dispatch(now);
	source.forget(peer, object);
	(void)source.dispatch(now);
	source.follow(peer, object);
	(void)source.dispatch(now);
	source.destroyObject(object);
	(void)source.dispatch(now);
	source.registerObject(object, entity);
	source.follow(peer, object);
	(void)source.dispatch(now);
	ASSERT_EQ(source.sent.size(), 5u);
	EXPECT_EQ(protocol.decodeUpdate(source.sent[1].second).edit, Edit::Forget);
	EXPECT_GT(protocol.decodeUpdate(source.sent[2].second).tracking, protocol.decodeUpdate(source.sent[0].second).tracking);
	EXPECT_EQ(protocol.decodeUpdate(source.sent[3].second).edit, Edit::Destroy);
	Replicas replicas;
	replicas.resetSession(session);
	for (const auto &[recipient, message] : source.sent)
	{
		EXPECT_TRUE(replicas.receiveMessage(message));
	}
	EXPECT_EQ(replicas.objects.size(), 1u);
	EXPECT_EQ(replicas.creations, 3);
	EXPECT_FALSE(replicas.receiveMessage(source.sent[3].second));
}

TEST_F(NetworkTraitsTest, CapacityFailureRetainsQueuedRemovalUntilDrained)
{
	Source source({.maximumObjects = 1, .maximumPeers = 1, .interval = 0ms});
	session = source.openPeer(peer);
	EXPECT_THROW((void)source.openPeer(ID::generate()), spk::Exception);
	Object first, second;
	source.registerObject(object, first);
	source.follow(peer, object);
	source.destroyObject(object);
	const auto other = ID::generate();
	source.registerObject(other, second);
	EXPECT_THROW(source.follow(peer, other), spk::Exception);
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	EXPECT_EQ(protocol.decodeUpdate(source.sent.back().second).edit, Edit::Destroy);
	EXPECT_NO_THROW(source.follow(peer, other));
	Replicas replicas(1);
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.receiveMessage(source.sent.back().second));
	(void)source.dispatch(now);
	EXPECT_THROW((void)replicas.receiveMessage(source.sent.back().second), spk::Exception);
}

TEST_F(NetworkTraitsTest, PerPeerBudgetRotatesEvenWhenOnePeerIsBlocked)
{
	Source source({.interval = 0ms});
	Object entity;
	const auto other = ID::generate();
	source.registerObject(object, entity);
	(void)source.openPeer(peer);
	(void)source.openPeer(other);
	source.follow(peer, object);
	source.follow(other, object);
	source.blocked.insert(peer);
	EXPECT_EQ(source.dispatch(now, 1).sent, 0u);
	EXPECT_EQ(source.dispatch(now, 1).sent, 1u);
	EXPECT_EQ(source.sent.back().first, other);
	source.blocked.clear();
	EXPECT_EQ(source.dispatch(now, 1).sent, 1u);
}
