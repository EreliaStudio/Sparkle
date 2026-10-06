#include "network/network_traits_test.hpp"
using namespace std::chrono_literals;

TEST_F(NetworkTraitsTest, RequestedStateCreatesReplicaAndCompletesAcquisitionWithoutReadyReply)
{
	RequestSource source;
	RequestReplicas replicas;
	session = source.openPeer(peer);
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.requestObject(object));
	ASSERT_TRUE(source.receiveMessage(peer, replicas.sent.back()));
	EXPECT_FALSE(source.receiveMessage(peer, replicas.sent.back()));
	ASSERT_TRUE(source.fulfillRequest(peer, source.requests.back().second, {24}));
	source.blocked.insert(peer);
	EXPECT_EQ(source.dispatch(now).sent, 0u);
	EXPECT_EQ(replicas.requestStatus(object), Status::Pending);
	source.blocked.clear();
	EXPECT_EQ(source.dispatch(now + 50ms).sent, 1u);
	ASSERT_EQ(source.sent.size(), 1u);
	EXPECT_EQ(source.sent.back().second.requestID(), replicas.sent.back().requestID());
	EXPECT_TRUE(replicas.receiveMessage(source.sent.back().second));
	EXPECT_EQ(replicas.objects.at(object).value, 24);
	EXPECT_EQ(replicas.creations, 1);
	EXPECT_EQ(replicas.requestStatus(object), Status::Ready);
}

TEST_F(NetworkTraitsTest, PlainSourceServesRegisteredObjectsAndRejectsMissingObjects)
{
	Source source;
	Object entity;
	entity.change(6);
	source.registerObject(object, entity);
	RequestReplicas replicas;
	replicas.resetSession(source.openPeer(peer));
	ASSERT_TRUE(replicas.requestObject(object));
	ASSERT_TRUE(source.receiveMessage(peer, replicas.sent.back()));
	(void)source.dispatch(now);
	ASSERT_TRUE(replicas.receiveMessage(source.sent.back().second));
	EXPECT_EQ(replicas.objects.at(object).value, 6);
	const auto missing = ID::generate();
	ASSERT_TRUE(replicas.requestObject(missing));
	ASSERT_TRUE(source.receiveMessage(peer, replicas.sent.back()));
	EXPECT_TRUE(replicas.receiveMessage(source.sent.back().second));
	EXPECT_EQ(replicas.requestStatus(missing), Status::Failed);
	EXPECT_FALSE(replicas.objects.contains(missing));
}

TEST_F(NetworkTraitsTest, ProviderMayCompleteSynchronouslyOrAfterRegisteringAnObject)
{
	RequestSource source;
	session = source.openPeer(peer);
	const Request first{session, object, 1};
	source.immediate = true;
	ASSERT_TRUE(source.receiveMessage(peer, protocol.encode(first)));
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	EXPECT_EQ(protocol.decodeUpdate(source.sent.back().second).state->value, 37);
	source.immediate = false;
	const Request second{session, ID::generate(), 2};
	ASSERT_TRUE(source.receiveMessage(peer, protocol.encode(second)));
	Object entity;
	entity.change(12);
	source.registerObject(second.object, entity);
	EXPECT_TRUE(source.acceptRequest(peer, second));
	EXPECT_EQ(source.dispatch(now + 50ms).sent, 1u);
	EXPECT_EQ(protocol.decodeUpdate(source.sent.back().second).state->value, 12);
}

TEST_F(NetworkTraitsTest, ClosingPeerCancelsProviderCompletionAndQueuedState)
{
	RequestSource source;
	session = source.openPeer(peer);
	const Request request{session, object, 1};
	ASSERT_TRUE(source.receiveMessage(peer, protocol.encode(request)));
	EXPECT_TRUE(source.fulfillRequest(peer, request, {1}));
	source.closePeer(peer);
	EXPECT_FALSE(source.fulfillRequest(peer, request, {99}));
	EXPECT_EQ(source.dispatch(now).sent, 0u);
	const auto nextSession = source.openPeer(peer);
	EXPECT_NE(nextSession, session);
	EXPECT_FALSE(source.receiveMessage(peer, protocol.encode(request)));
	const Request next{nextSession, object, 1};
	ASSERT_TRUE(source.receiveMessage(peer, protocol.encode(next)));
	EXPECT_TRUE(source.fulfillRequest(peer, next, {2}));
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	EXPECT_EQ(protocol.decodeUpdate(source.sent.back().second).state->value, 2);
}

TEST_F(NetworkTraitsTest, SupersededProviderResultCannotPublish)
{
	RequestSource source;
	session = source.openPeer(peer);
	const Request first{session, object, 1}, next{session, object, 2};
	ASSERT_TRUE(source.receiveMessage(peer, protocol.encode(first)));
	ASSERT_TRUE(source.receiveMessage(peer, protocol.encode(next)));
	EXPECT_FALSE(source.fulfillRequest(peer, first, {99}));
	EXPECT_TRUE(source.fulfillRequest(peer, next, {2}));
	EXPECT_FALSE(source.fulfillRequest(peer, next, {3}));
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	EXPECT_EQ(protocol.decodeUpdate(source.sent.back().second).state->value, 2);
}

TEST_F(NetworkTraitsTest, FailedProviderCanRejectAndClientExplicitlyRequestsAgain)
{
	RequestSource source;
	RequestReplicas replicas;
	replicas.resetSession(source.openPeer(peer));
	ASSERT_TRUE(replicas.requestObject(object));
	source.failRequest = true;
	EXPECT_THROW((void)source.receiveMessage(peer, replicas.sent.back()), spk::Exception);
	EXPECT_TRUE(source.rejectRequest(peer, source.requests.back().second));
	EXPECT_TRUE(replicas.receiveMessage(source.sent.back().second));
	EXPECT_EQ(replicas.requestStatus(object), Status::Failed);
	ASSERT_TRUE(replicas.requestObject(object));
	source.failRequest = false;
	source.immediate = true;
	ASSERT_TRUE(source.receiveMessage(peer, replicas.sent.back()));
	(void)source.dispatch(now);
	EXPECT_TRUE(replicas.receiveMessage(source.sent.back().second));
	EXPECT_EQ(replicas.requestStatus(object), Status::Ready);
}

TEST_F(NetworkTraitsTest, FailedRequestSendPreservesStatusAndCanBeRetriedExplicitly)
{
	RequestReplicas replicas;
	replicas.resetSession(session);
	replicas.blocked = true;
	EXPECT_FALSE(replicas.requestObject(object));
	EXPECT_FALSE(replicas.requestStatus(object));
	replicas.blocked = false;
	replicas.throwSend = true;
	EXPECT_THROW((void)replicas.requestObject(object), spk::Exception);
	EXPECT_FALSE(replicas.requestStatus(object));
	replicas.throwSend = false;
	ASSERT_TRUE(replicas.requestObject(object));
	const auto request = protocol.decodeRequest(replicas.sent.back());
	replicas.blocked = true;
	EXPECT_FALSE(replicas.requestObject(object));
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(request, Protocol::Kind::Rejected)));
	EXPECT_EQ(replicas.requestStatus(object), Status::Failed);
}

TEST_F(NetworkTraitsTest, OldResponseCannotCompleteNewRequestButAuthoritativeStateStillApplies)
{
	RequestReplicas replicas;
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.requestObject(object));
	const auto old = replicas.sent.back().requestID();
	ASSERT_TRUE(replicas.requestObject(object));
	const auto current = replicas.sent.back().requestID();
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(update(4), old)));
	EXPECT_EQ(replicas.requestStatus(object), Status::Pending);
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(update(5, 2), current)));
	EXPECT_EQ(replicas.requestStatus(object), Status::Ready);
	EXPECT_EQ(replicas.objects.at(object).value, 5);
	replicas.cancelRequest(object);
	EXPECT_FALSE(replicas.requestStatus(object));
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(update(6, 3), current)));
	EXPECT_FALSE(replicas.requestStatus(object));
	EXPECT_EQ(replicas.objects.at(object).value, 6);
}

TEST_F(NetworkTraitsTest, RequestForExistingReplicaCompletesWithoutReapplyingIdenticalState)
{
	RequestReplicas replicas;
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.receiveMessage(protocol.encode(update(7))));
	ASSERT_TRUE(replicas.requestObject(object));
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(update(7), replicas.sent.back().requestID())));
	EXPECT_EQ(replicas.requestStatus(object), Status::Ready);
	EXPECT_EQ(replicas.objects.at(object).applications, 1);
}

TEST_F(NetworkTraitsTest, RequestTransportCannotResetOrMutateCollectionDuringSend)
{
	RequestReplicas replicas;
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.receiveMessage(protocol.encode(update(4))));
	replicas.onSend = [&] {
		replicas.resetSession(ID::generate());
	};
	EXPECT_THROW((void)replicas.requestObject(object), spk::Exception);
	EXPECT_EQ(replicas.objects.at(object).value, 4);
	EXPECT_FALSE(replicas.requestStatus(object));
	replicas.onSend = {};
	ASSERT_TRUE(replicas.requestObject(object));
	EXPECT_EQ(protocol.decodeRequest(replicas.sent.back()).session, session);
}

TEST_F(NetworkTraitsTest, DisconnectClearsReplicasAndRequestsAndRequiresNewHandshake)
{
	RequestReplicas replicas;
	replicas.resetSession(session);
	const auto message = protocol.encode(update(4));
	ASSERT_TRUE(replicas.receiveMessage(message));
	ASSERT_TRUE(replicas.requestObject(object));
	replicas.closeSession();
	EXPECT_TRUE(replicas.objects.empty());
	EXPECT_FALSE(replicas.requestStatus(object));
	EXPECT_FALSE(replicas.receiveMessage(message));
	EXPECT_THROW((void)replicas.requestObject(object), spk::Exception);
	EXPECT_NO_THROW(replicas.closeSession());
	session = ID::generate();
	replicas.resetSession(session);
	EXPECT_TRUE(replicas.requestObject(object));
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(update(5))));
}

TEST_F(NetworkTraitsTest, FailedApplicationDoesNotCompleteAcquisition)
{
	RequestReplicas replicas;
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.requestObject(object));
	const auto message = protocol.encode(update(10), replicas.sent.back().requestID());
	replicas.failInitialApply = true;
	EXPECT_THROW((void)replicas.receiveMessage(message), spk::Exception);
	EXPECT_EQ(replicas.requestStatus(object), Status::Pending);
	EXPECT_TRUE(replicas.objects.empty());
	replicas.failInitialApply = false;
	ASSERT_TRUE(replicas.receiveMessage(message));
	EXPECT_EQ(replicas.requestStatus(object), Status::Ready);
}

TEST_F(NetworkTraitsTest, CoalescedStateRetainsAcquisitionCorrelation)
{
	Source source;
	Object entity;
	source.registerObject(object, entity);
	RequestReplicas replicas;
	replicas.resetSession(source.openPeer(peer));
	ASSERT_TRUE(replicas.requestObject(object));
	ASSERT_TRUE(source.receiveMessage(peer, replicas.sent.back()));
	entity.change(42);
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	EXPECT_TRUE(replicas.receiveMessage(source.sent.back().second));
	EXPECT_EQ(replicas.objects.at(object).value, 42);
	EXPECT_EQ(replicas.requestStatus(object), Status::Ready);
}

TEST_F(NetworkTraitsTest, ForgetCancelsPendingProviderAndRemovesAcquisitionStatus)
{
	RequestSource source;
	Object entity;
	source.registerObject(object, entity);
	RequestReplicas replicas;
	replicas.resetSession(source.openPeer(peer));
	source.follow(peer, object);
	(void)source.dispatch(now);
	ASSERT_TRUE(replicas.receiveMessage(source.sent.back().second));
	ASSERT_TRUE(replicas.requestObject(object));
	ASSERT_TRUE(source.receiveMessage(peer, replicas.sent.back()));
	source.forget(peer, object);
	EXPECT_FALSE(source.fulfillRequest(peer, source.requests.back().second, {99}));
	(void)source.dispatch(now + 50ms);
	EXPECT_TRUE(replicas.receiveMessage(source.sent.back().second));
	EXPECT_TRUE(replicas.objects.empty());
	EXPECT_FALSE(replicas.requestStatus(object));
}

TEST_F(NetworkTraitsTest, RejectionSendCanRetryAndRejectsReentry)
{
	RequestSource source;
	session = source.openPeer(peer);
	const Request request{session, object, 1};
	ASSERT_TRUE(source.receiveMessage(peer, protocol.encode(request)));
	source.blocked.insert(peer);
	EXPECT_FALSE(source.rejectRequest(peer, request));
	source.blocked.clear();
	source.onSend = [&] {
		source.closePeer(peer);
	};
	EXPECT_THROW((void)source.rejectRequest(peer, request), spk::Exception);
	source.onSend = {};
	EXPECT_TRUE(source.rejectRequest(peer, request));
	EXPECT_FALSE(source.rejectRequest(peer, request));
}

TEST_F(NetworkTraitsTest, OldSessionOrSupersededRejectionsCannotFailCurrentAcquisition)
{
	RequestReplicas replicas;
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.requestObject(object));
	const auto old = protocol.decodeRequest(replicas.sent.back());
	ASSERT_TRUE(replicas.requestObject(object));
	EXPECT_FALSE(replicas.receiveMessage(protocol.encode(old, Protocol::Kind::Rejected)));
	EXPECT_EQ(replicas.requestStatus(object), Status::Pending);
	replicas.resetSession(ID::generate());
	ASSERT_TRUE(replicas.requestObject(object));
	EXPECT_FALSE(replicas.receiveMessage(protocol.encode(old, Protocol::Kind::Rejected)));
	EXPECT_EQ(replicas.requestStatus(object), Status::Pending);
}

TEST_F(NetworkTraitsTest, AcquisitionLimitsFailBeforeSendingOrStartingAnotherProvider)
{
	RequestSource source({.maximumObjects = 1});
	RequestReplicas replicas(1);
	session = source.openPeer(peer);
	replicas.resetSession(session);
	ASSERT_TRUE(replicas.requestObject(object));
	ASSERT_TRUE(source.receiveMessage(peer, replicas.sent.back()));
	const auto other = ID::generate();
	EXPECT_THROW((void)replicas.requestObject(other), spk::Exception);
	EXPECT_EQ(replicas.sent.size(), 1u);
	const Request extra{session, other, 99};
	EXPECT_THROW((void)source.receiveMessage(peer, protocol.encode(extra)), spk::Exception);
	EXPECT_EQ(source.requests.size(), 1u);
	ASSERT_TRUE(source.rejectRequest(peer, source.requests.back().second));
	EXPECT_TRUE(source.receiveMessage(peer, protocol.encode(extra)));
	replicas.cancelRequest(object);
	EXPECT_TRUE(replicas.requestObject(other));
}

TEST_F(NetworkTraitsTest, RequestCorrelationAndDuplicateHistoryAreIndependentPerPeer)
{
	RequestSource source;
	const auto other = ID::generate();
	const Request first{source.openPeer(peer), object, 1};
	const Request second{source.openPeer(other), object, 1};
	ASSERT_TRUE(source.receiveMessage(peer, protocol.encode(first)));
	ASSERT_TRUE(source.receiveMessage(other, protocol.encode(second)));
	ASSERT_TRUE(source.fulfillRequest(peer, first, {1}));
	ASSERT_TRUE(source.fulfillRequest(other, second, {2}));
	EXPECT_FALSE(source.receiveMessage(peer, protocol.encode(first)));
	EXPECT_FALSE(source.receiveMessage(other, protocol.encode(second)));
	EXPECT_EQ(source.dispatch(now).sent, 2u);
	for (const auto &[recipient, message] : source.sent)
	{
		EXPECT_EQ(message.requestID(), 1u);
		EXPECT_EQ(protocol.decodeUpdate(message).session, recipient == peer ? first.session : second.session);
		EXPECT_EQ(protocol.decodeUpdate(message).state->value, 2);
	}
}
