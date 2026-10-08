#include "network/trait/incoming_request_history_trait.hpp"
#include "network/trait/publication_fairness_trait.hpp"
#include "network/trait/replica_revision_trait.hpp"
#include "exception.hpp"
#include <gtest/gtest.h>

namespace
{
	class IncomingHistory : public spk::Network::IncomingRequestHistoryTrait
	{
	public:
		using IncomingRequestHistoryTrait::_clearRequestHistory;
		using IncomingRequestHistoryTrait::_isNewRequest;
		using IncomingRequestHistoryTrait::_recordRequest;
	};
	class Fairness : public spk::Network::PublicationFairnessTrait
	{
	public:
		using PublicationFairnessTrait::_hasScheduledPeers;
		using PublicationFairnessTrait::_nextScheduledPeer;
		using PublicationFairnessTrait::_schedulePeer;
		using PublicationFairnessTrait::_scheduledPeerCount;
		using PublicationFairnessTrait::_unschedulePeer;
	};
	class Revision : public spk::Network::ReplicaRevisionTrait
	{
	public:
		using ReplicaRevisionTrait::_obsoleteRevision;
	};
}

TEST(IncomingRequestHistoryTraitTest, RejectsStaleIdsIndependentlyPerPeerAndResetsOnClose)
{
	IncomingHistory history;
	const auto first = spk::UUID::generate();
	const auto second = spk::UUID::generate();
	EXPECT_TRUE(history._isNewRequest(first, 1));
	history._recordRequest(first, 7);
	EXPECT_FALSE(history._isNewRequest(first, 7));
	EXPECT_FALSE(history._isNewRequest(first, 6));
	EXPECT_TRUE(history._isNewRequest(first, 8));
	EXPECT_TRUE(history._isNewRequest(second, 1));
	history._clearRequestHistory(first);
	EXPECT_TRUE(history._isNewRequest(first, 1));
}
TEST(PublicationFairnessTraitTest, RotatesPeersAndForgetsClosedConnections)
{
	Fairness fairness;
	const auto first = spk::UUID::generate();
	const auto second = spk::UUID::generate();
	EXPECT_FALSE(fairness._hasScheduledPeers());
	EXPECT_THROW((void)fairness._nextScheduledPeer(), spk::Exception);
	fairness._schedulePeer(first);
	fairness._schedulePeer(second);
	EXPECT_EQ(fairness._scheduledPeerCount(), 2u);
	EXPECT_EQ(fairness._nextScheduledPeer(), first);
	EXPECT_EQ(fairness._nextScheduledPeer(), second);
	fairness._unschedulePeer(first);
	EXPECT_EQ(fairness._scheduledPeerCount(), 1u);
	EXPECT_EQ(fairness._nextScheduledPeer(), second);
}
TEST(ReplicaRevisionTraitTest, RetainsTrackingAndTombstoneOrdering)
{
	using spk::Network::Edit;
	EXPECT_TRUE(Revision::_obsoleteRevision(9, 3, true, 8, 99, Edit::Set));
	EXPECT_TRUE(Revision::_obsoleteRevision(9, 3, true, 9, 3, Edit::Set));
	EXPECT_FALSE(Revision::_obsoleteRevision(9, 3, true, 9, 4, Edit::Set));
	EXPECT_FALSE(Revision::_obsoleteRevision(9, 3, true, 10, 1, Edit::Set));
	EXPECT_TRUE(Revision::_obsoleteRevision(9, 3, false, 9, 4, Edit::Set));
}
