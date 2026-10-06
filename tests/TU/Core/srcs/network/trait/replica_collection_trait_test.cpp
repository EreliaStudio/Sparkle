#include "network/network_traits_test.hpp"

TEST_F(NetworkTraitsTest, ReplicaTraitAppliesThroughInheritedOperation)
{
	Object entity;
	entity.applyNetworkState({19});
	EXPECT_EQ(entity.value, 19);
	EXPECT_EQ(entity.applications, 1);
	entity.failApply = true;
	EXPECT_THROW(entity.applyNetworkState({20}), spk::Exception);
	EXPECT_EQ(entity.value, 19);
	entity.failApply = false;
	entity.applyNetworkState({20});
	EXPECT_EQ(entity.applications, 2);
}

TEST_F(NetworkTraitsTest, CollectionPreservesTombstonesAfterRemovingReplica)
{
	Replicas replicas;
	replicas.resetSession(session);
	const auto initial = protocol.encode(update(1));
	EXPECT_TRUE(replicas.receiveMessage(initial));
	auto stop = update(0);
	stop.edit = Edit::Forget;
	stop.state.reset();
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(stop)));
	EXPECT_TRUE(replicas.objects.empty());
	EXPECT_FALSE(replicas.receiveMessage(initial));
	EXPECT_FALSE(replicas.receiveMessage(protocol.encode(update(10, 99))));
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(update(3, 1, 2))));
	EXPECT_FALSE(replicas.receiveMessage(protocol.encode(stop)));
	EXPECT_EQ(replicas.objects.at(object).value, 3);
}

TEST_F(NetworkTraitsTest, SessionTransitionRemovesReplicasAndRejectsOldTraffic)
{
	Replicas replicas;
	replicas.resetSession(session);
	const auto message = protocol.encode(update(8));
	EXPECT_TRUE(replicas.receiveMessage(message));
	replicas.resetSession(session);
	EXPECT_EQ(replicas.removals, 0);
	replicas.resetSession(ID::generate());
	EXPECT_TRUE(replicas.objects.empty());
	EXPECT_EQ(replicas.removals, 1);
	EXPECT_FALSE(replicas.receiveMessage(message));
	EXPECT_THROW(replicas.resetSession({}), spk::Exception);
}

TEST_F(NetworkTraitsTest, FailedApplicationDoesNotConsumeRevisionOrCapacity)
{
	Replicas replicas(1);
	replicas.resetSession(session);
	replicas.failApply = true;
	EXPECT_THROW((void)replicas.receiveMessage(protocol.encode(update(1))), spk::Exception);
	object = ID::generate();
	replicas.failApply = false;
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(update(2))));
	replicas.failApply = true;
	EXPECT_THROW((void)replicas.receiveMessage(protocol.encode(update(3, 2))), spk::Exception);
	replicas.failApply = false;
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(update(3, 2))));
	EXPECT_EQ(replicas.objects.at(object).value, 3);
}

TEST_F(NetworkTraitsTest, FailedRemovalAndResetCanRetry)
{
	Replicas replicas;
	replicas.resetSession(session);
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(update(8))));
	replicas.failRemove = true;
	auto stop = update(0);
	stop.edit = Edit::Destroy;
	stop.state.reset();
	EXPECT_THROW((void)replicas.receiveMessage(protocol.encode(stop)), spk::Exception);
	EXPECT_THROW(replicas.resetSession(ID::generate()), spk::Exception);
	replicas.failRemove = false;
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(stop)));
	EXPECT_TRUE(replicas.objects.empty());
	EXPECT_FALSE(replicas.receiveMessage(protocol.encode(update(9, 2))));
}

TEST_F(NetworkTraitsTest, CollectionRejectsHookReentryBeforeChangingHistory)
{
	Replicas replicas;
	replicas.resetSession(session);
	replicas.onApply = [&] {
		replicas.resetSession(ID::generate());
	};
	const auto message = protocol.encode(update(7));
	EXPECT_THROW((void)replicas.receiveMessage(message), spk::Exception);
	replicas.onApply = {};
	EXPECT_TRUE(replicas.receiveMessage(message));
	EXPECT_EQ(replicas.objects.at(object).value, 7);
}

TEST_F(NetworkTraitsTest, UnsolicitedStateCreatesThenUpdatesTheSameReplica)
{
	Replicas replicas;
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.receiveMessage(protocol.encode(update(10))));
	auto *original = &replicas.objects.at(object);
	ASSERT_TRUE(replicas.receiveMessage(protocol.encode(update(20, 2))));
	EXPECT_EQ(&replicas.objects.at(object), original);
	EXPECT_EQ(replicas.creations, 1);
	EXPECT_EQ(original->value, 20);
	auto stop = update(0, 2);
	stop.edit = Edit::Destroy;
	stop.state.reset();
	ASSERT_TRUE(replicas.receiveMessage(protocol.encode(stop)));
	EXPECT_TRUE(replicas.objects.empty());
	EXPECT_FALSE(replicas.receiveMessage(protocol.encode(update(30, 3))));
}

TEST_F(NetworkTraitsTest, FailedInitialStateRemovesNewReplicaAndCanRetryTheSameRevision)
{
	Replicas replicas(1);
	replicas.resetSession(session);
	replicas.failInitialApply = true;
	const auto message = protocol.encode(update(10));
	EXPECT_THROW((void)replicas.receiveMessage(message), spk::Exception);
	EXPECT_TRUE(replicas.objects.empty());
	replicas.failInitialApply = false;
	EXPECT_TRUE(replicas.receiveMessage(message));
	EXPECT_EQ(replicas.objects.at(object).value, 10);
	EXPECT_EQ(replicas.creations, 2);
}
