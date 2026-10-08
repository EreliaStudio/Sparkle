#include "network/network_traits_test.hpp"

class PublicationSnapshotTraitTest : public NetworkTraitsTest
{
protected:
	class Snapshots : public spk::Network::PublicationSnapshotTrait
	{
	public:
		using PublicationSnapshotTrait::_eraseSnapshot;
		using PublicationSnapshotTrait::_hasSnapshot;
		using PublicationSnapshotTrait::_publish;
		using PublicationSnapshotTrait::_snapshot;
		using PublicationSnapshotTrait::PublicationSnapshotTrait;
	};
	class Observer : public spk::Network::PublishedObjectObservationTrait
	{
		void _publishObserved(ID, spk::Message payload) override
		{
			if (failPublication)
			{
				throw spk::Exception("Publication failed");
			}
			captured.push_back(std::move(payload));
		}

	public:
		using PublishedObjectObservationTrait::_captureChanges;
		using PublishedObjectObservationTrait::_registerObject;
		using PublishedObjectObservationTrait::_unregisterObject;
		bool failPublication = false;
		std::vector<spk::Message> captured;
	};
};

TEST_F(PublicationSnapshotTraitTest, RetainsPayloadWithoutLiveObjectsAndRevisionsSurviveErasure)
{
	Snapshots snapshots(1);
	snapshots._publish(object, payload(1));
	const auto first = snapshots._snapshot(object);
	snapshots._publish(object, payload(2));
	EXPECT_GT(snapshots._snapshot(object).revision, first.revision);
	EXPECT_EQ(first.payload->reader().get<int>(), 1);
	const auto other = ID::generate();
	EXPECT_THROW(snapshots._publish(other, payload(3)), spk::Exception);
	EXPECT_FALSE(snapshots._hasSnapshot(other));
	const auto revision = snapshots._snapshot(object).revision;
	snapshots._eraseSnapshot(object);
	snapshots._publish(other, payload(3));
	EXPECT_GT(snapshots._snapshot(other).revision, revision);
}

TEST_F(PublicationSnapshotTraitTest, FailedPublicationDoesNotConsumeObservedVersion)
{
	Observer observer;
	Object entity;
	observer._registerObject(object, entity);
	entity.change(5);
	observer.failPublication = true;
	EXPECT_THROW(observer._captureChanges(), spk::Exception);
	ASSERT_EQ(observer.captured.size(), 1u);
	observer.failPublication = false;
	observer._captureChanges();
	ASSERT_EQ(observer.captured.size(), 2u);
	EXPECT_EQ(observer.captured.back().reader().get<int>(), 5);
	observer._captureChanges();
	EXPECT_EQ(observer.captured.size(), 2u);
	EXPECT_EQ(entity.builds, 3);
}

TEST_F(PublicationSnapshotTraitTest, ObserversRemainIndependentAndExpiredObjectsAreNotRead)
{
	Observer first, second;
	{
		Object entity;
		first._registerObject(object, entity);
		second._registerObject(object, entity);
		entity.change(8);
		first._captureChanges();
		EXPECT_EQ(second.captured.size(), 1u);
		second._captureChanges();
		first._unregisterObject(object);
		entity.change(9);
		first._captureChanges();
		second._captureChanges();
	}
	EXPECT_NO_THROW(first._captureChanges());
	EXPECT_NO_THROW(second._captureChanges());
	EXPECT_EQ(first.captured.back().reader().get<int>(), 8);
	EXPECT_EQ(second.captured.back().reader().get<int>(), 9);
}

TEST_F(PublicationSnapshotTraitTest, FailedInitialPublicationCanRegisterAndCaptureAgain)
{
	Observer observer;
	Object entity;
	observer.failPublication = true;
	EXPECT_THROW(observer._registerObject(object, entity), spk::Exception);
	observer.failPublication = false;
	EXPECT_NO_THROW(observer._registerObject(object, entity));
	EXPECT_THROW(observer._registerObject(object, entity), spk::Exception);
	entity.change(4);
	observer._captureChanges();
	ASSERT_EQ(observer.captured.size(), 2u);
	EXPECT_EQ(observer.captured.back().reader().get<int>(), 4);
}
