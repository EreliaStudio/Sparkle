#include "network/network_traits_test.hpp"
#include <concepts>
#include <type_traits>

using namespace std::chrono_literals;

TEST_F(NetworkTraitsTest, VersionNotificationsDoNotReplacePerSourceObservation)
{
	static_assert(std::derived_from<Object, spk::VersionedTrait>);
	static_assert(!std::is_copy_constructible_v<Object>);
	static_assert(!std::is_move_constructible_v<Object>);
	Source first, second;
	Object entity;
	first.registerObject(object, entity);
	second.registerObject(object, entity);
	(void)first.openPeer(peer);
	(void)second.openPeer(peer);
	first.follow(peer, object);
	second.follow(peer, object);
	(void)first.dispatch(now);
	(void)second.dispatch(now);
	spk::VersionedTrait &versioned = entity;
	int notifications = 0;
	auto contract = versioned.subscribeToVersionEdition([&](spk::VersionedTrait *edited) {
		EXPECT_EQ(edited, &versioned);
		++notifications;
		throw spk::Exception("Version subscriber failure");
	});
	EXPECT_THROW(entity.change(13), spk::Exception);
	EXPECT_EQ(versioned.version(), 1u);
	EXPECT_EQ(notifications, 1);
	EXPECT_EQ(first.dispatch(now + 50ms).sent, 1u);
	EXPECT_EQ(second.dispatch(now + 50ms).sent, 1u);
	EXPECT_EQ(protocol.decodeUpdate(first.sent.back().second).state->value, 13);
	EXPECT_EQ(protocol.decodeUpdate(second.sent.back().second).state->value, 13);
	contract.resign();
	EXPECT_NO_THROW(entity.change(14));
	EXPECT_EQ(versioned.version(), 2u);
	EXPECT_EQ(first.dispatch(now + 100ms).sent, 1u);
	EXPECT_EQ(second.dispatch(now + 100ms).sent, 1u);
}

TEST_F(NetworkTraitsTest, ReplacementObjectWithSameVersionStillPublishesANewerRevision)
{
	Source source;
	Object first, replacement;
	first.change(1);
	replacement.change(2);
	ASSERT_EQ(first.version(), replacement.version());
	source.registerObject(object, first);
	session = source.openPeer(peer);
	source.follow(peer, object);
	ASSERT_EQ(source.dispatch(now).sent, 1u);
	const auto initial = protocol.decodeUpdate(source.sent.back().second);
	source.unregisterObject(object);
	source.registerObject(object, replacement);
	ASSERT_EQ(source.dispatch(now + 50ms).sent, 1u);
	const auto next = protocol.decodeUpdate(source.sent.back().second);
	EXPECT_EQ(next.tracking, initial.tracking);
	EXPECT_GT(next.revision, initial.revision);
	Replicas replicas;
	replicas.resetSession(session);
	EXPECT_TRUE(replicas.receiveMessage(source.sent.front().second));
	EXPECT_TRUE(replicas.receiveMessage(source.sent.back().second));
	EXPECT_EQ(replicas.objects.at(object).value, 2);
}

TEST_F(NetworkTraitsTest, PublishesInheritedStateAndCoalescesBeforeSnapshotCapture)
{
	Source source;
	Object entity;
	entity.change(1);
	source.registerObject(object, entity);
	session = source.openPeer(peer);
	source.follow(peer, object);
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	entity.change(2);
	entity.change(3);
	EXPECT_EQ(source.dispatch(now + 49ms).sent, 0u);
	EXPECT_EQ(entity.builds, 1);
	EXPECT_EQ(source.dispatch(now + 50ms).sent, 1u);
	EXPECT_EQ(entity.builds, 2);
	Replicas replicas;
	replicas.resetSession(session);
	for (const auto &[recipient, message] : source.sent)
	{
		EXPECT_EQ(recipient, peer);
		EXPECT_TRUE(replicas.receiveMessage(message));
	}
	EXPECT_EQ(replicas.objects.at(object).value, 3);
	EXPECT_EQ(replicas.objects.at(object).applications, 2);
	EXPECT_EQ(Codec::decodes, 2);
	EXPECT_EQ(source.dispatch(now + 100ms).sent, 0u);
	EXPECT_EQ(entity.builds, 2);
}

TEST_F(NetworkTraitsTest, LocalDetachKeepsSnapshotUntilExplicitDestruction)
{
	Source source;
	Object entity;
	source.registerObject(object, entity);
	session = source.openPeer(peer);
	source.follow(peer, object);
	(void)source.dispatch(now);
	source.unregisterObject(object);
	entity.change(9);
	EXPECT_EQ(source.dispatch(now + 50ms).sent, 0u);
	EXPECT_EQ(entity.builds, 1);
	source.destroyObject(object);
	EXPECT_EQ(source.dispatch(now + 100ms).sent, 1u);
	EXPECT_EQ(protocol.decodeUpdate(source.sent.back().second).edit, Edit::Destroy);
}

TEST_F(NetworkTraitsTest, EitherLifetimeOrderDetachesSafely)
{
	Source source;
	{
		Object entity;
		entity.change(8);
		source.registerObject(object, entity);
		entity.change(12);
	}
	session = source.openPeer(peer);
	source.follow(peer, object);
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	EXPECT_EQ(protocol.decodeUpdate(source.sent.back().second).state->value, 8);
	Object survivor;
	{
		Source temporary;
		temporary.registerObject(object, survivor);
	}
	EXPECT_NO_THROW(survivor.change(5));
	source.registerObject(object, survivor);
	EXPECT_EQ(source.dispatch(now + 50ms).sent, 1u);
	EXPECT_EQ(protocol.decodeUpdate(source.sent.back().second).state->value, 5);
}

TEST_F(NetworkTraitsTest, FailedSnapshotCaptureCanRetryAndDuplicateRegistrationIsRejected)
{
	Source source;
	Object entity;
	entity.failBuild = true;
	EXPECT_THROW(source.registerObject(object, entity), spk::Exception);
	entity.failBuild = false;
	source.registerObject(object, entity);
	EXPECT_THROW(source.registerObject(object, entity), spk::Exception);
	session = source.openPeer(peer);
	source.follow(peer, object);
	entity.failBuild = true;
	entity.change(7);
	EXPECT_THROW(source.dispatch(now), spk::Exception);
	entity.failBuild = false;
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	EXPECT_EQ(protocol.decodeUpdate(source.sent.back().second).state->value, 7);
}

TEST_F(NetworkTraitsTest, BlockedAndThrowingPeersDoNotBlockOthers)
{
	Source source;
	Object entity;
	const auto other = ID::generate();
	source.registerObject(object, entity);
	session = source.openPeer(peer);
	(void)source.openPeer(other);
	source.follow(peer, object);
	source.follow(other, object);
	source.blocked.insert(peer);
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	EXPECT_EQ(source.sent.front().first, other);
	source.blocked.clear();
	source.throwing.insert(peer);
	EXPECT_EQ(source.dispatch(now + 50ms).errors, 1u);
	source.throwing.clear();
	EXPECT_EQ(source.dispatch(now + 100ms).sent, 1u);
}

TEST_F(NetworkTraitsTest, PublicationRejectsSenderReentryAndRecovers)
{
	Source source;
	Object entity;
	source.registerObject(object, entity);
	session = source.openPeer(peer);
	source.follow(peer, object);
	source.onSend = [&] {
		source.closePeer(peer);
	};
	EXPECT_EQ(source.dispatch(now).errors, 1u);
	source.onSend = {};
	EXPECT_EQ(source.dispatch(now + 50ms).sent, 1u);
	source.closePeer(peer);
	EXPECT_THROW(source.follow(peer, object), spk::Exception);
}

TEST_F(NetworkTraitsTest, EachSourceObservesInvalidationIndependently)
{
	Source first, second;
	Object entity;
	first.registerObject(object, entity);
	second.registerObject(object, entity);
	(void)first.openPeer(peer);
	(void)second.openPeer(peer);
	first.follow(peer, object);
	second.follow(peer, object);
	(void)first.dispatch(now);
	(void)second.dispatch(now);
	entity.change(13);
	EXPECT_EQ(first.dispatch(now + 50ms).sent, 1u);
	EXPECT_EQ(second.dispatch(now + 50ms).sent, 1u);
	EXPECT_EQ(protocol.decodeUpdate(first.sent.back().second).state->value, 13);
	EXPECT_EQ(protocol.decodeUpdate(second.sent.back().second).state->value, 13);
}
