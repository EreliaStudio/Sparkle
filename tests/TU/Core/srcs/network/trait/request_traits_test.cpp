#include "network/network_traits_test.hpp"

using namespace std::chrono_literals;

TEST_F(NetworkTraitsTest, RequestTraitsOrderInitialSnapshotBeforeReady)
{
	RequestSource source;
	RequestReplicas replicas;
	session = source.openPeer(peer);
	replicas.resetSession(session);
	replicas.requestObject(object, now);
	EXPECT_EQ(replicas.dispatchRequests(now), 1u);
	EXPECT_TRUE(source.receiveMessage(peer, replicas.sent.front()));
	ASSERT_EQ(source.requests.size(), 1u);
	EXPECT_FALSE(source.receiveMessage(peer, replicas.sent.front()));
	EXPECT_TRUE(source.fulfillRequest(peer, source.requests.front().second, {24}));
	source.blocked = true;
	EXPECT_EQ(source.dispatch(now).sent, 0u);
	EXPECT_TRUE(source.sent.empty());
	EXPECT_EQ(replicas.requestStatus(object), Status::Pending);
	source.blocked = false;
	EXPECT_EQ(source.dispatch(now + 50ms).sent, 1u);
	ASSERT_EQ(source.sent.size(), 2u);
	EXPECT_EQ(protocol.kind(source.sent[0].second), Protocol::Kind::Update);
	EXPECT_EQ(protocol.kind(source.sent[1].second), Protocol::Kind::Reply);
	EXPECT_TRUE(replicas.receiveMessage(source.sent[0].second, now + 50ms));
	EXPECT_EQ(replicas.requestStatus(object), Status::Pending);
	EXPECT_TRUE(replicas.receiveMessage(source.sent[1].second, now + 50ms));
	EXPECT_EQ(replicas.requestStatus(object), Status::Ready);
	EXPECT_EQ(replicas.objects.at(object).value, 24);
}

TEST_F(NetworkTraitsTest, RequestCanAcceptRegisteredObjectAndCompleteSynchronously)
{
	RequestSource source;
	Object entity;
	entity.change(6);
	source.registerObject(object, entity);
	session = source.openPeer(peer);
	const Request request{session, object, 1};
	EXPECT_TRUE(source.receiveMessage(peer, protocol.encode(request)));
	EXPECT_TRUE(source.acceptRequest(peer, request));
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	ASSERT_EQ(source.sent.size(), 2u);
	EXPECT_EQ(protocol.decodeUpdate(source.sent.front().second).state->value, 6);
	const Request immediate{session, ID::generate(), 2};
	source.immediate = true;
	EXPECT_TRUE(source.receiveMessage(peer, protocol.encode(immediate)));
	EXPECT_EQ(source.dispatch(now + 50ms).sent, 1u);
	EXPECT_EQ(source.sent.size(), 4u);
}

TEST_F(NetworkTraitsTest, ClosingPeerCancelsProviderCompletionAndQueuedReady)
{
	RequestSource source;
	session = source.openPeer(peer);
	const Request request{session, object, 1};
	EXPECT_TRUE(source.receiveMessage(peer, protocol.encode(request)));
	EXPECT_TRUE(source.fulfillRequest(peer, request, {1}));
	source.closePeer(peer);
	EXPECT_FALSE(source.fulfillRequest(peer, request, {99}));
	EXPECT_EQ(source.dispatch(now).sent, 0u);
	EXPECT_TRUE(source.sent.empty());
	const auto nextSession = source.openPeer(peer);
	EXPECT_NE(nextSession, session);
	EXPECT_FALSE(source.receiveMessage(peer, protocol.encode(request)));
	const Request next{nextSession, object, 1};
	EXPECT_TRUE(source.receiveMessage(peer, protocol.encode(next)));
	EXPECT_TRUE(source.fulfillRequest(peer, next, {2}));
	EXPECT_EQ(source.dispatch(now + 50ms).sent, 1u);
	EXPECT_EQ(protocol.decodeUpdate(source.sent.front().second).state->value, 2);
}

TEST_F(NetworkTraitsTest, SupersededProviderResultCannotPublish)
{
	RequestSource source;
	session = source.openPeer(peer);
	const Request first{session, object, 1}, next{session, object, 2};
	EXPECT_TRUE(source.receiveMessage(peer, protocol.encode(first)));
	EXPECT_TRUE(source.receiveMessage(peer, protocol.encode(next)));
	EXPECT_FALSE(source.fulfillRequest(peer, first, {99}));
	EXPECT_TRUE(source.fulfillRequest(peer, next, {2}));
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	EXPECT_EQ(protocol.decodeUpdate(source.sent.front().second).state->value, 2);
}

TEST_F(NetworkTraitsTest, FailedProviderProducesRetryAndCanRecover)
{
	RequestSource source;
	RequestReplicas replicas;
	session = source.openPeer(peer);
	replicas.resetSession(session);
	replicas.requestObject(object, now);
	EXPECT_EQ(replicas.dispatchRequests(now), 1u);
	source.failRequest = true;
	EXPECT_THROW((void)source.receiveMessage(peer, replicas.sent.front()), spk::Exception);
	(void)source.dispatch(now);
	ASSERT_EQ(source.sent.size(), 1u);
	EXPECT_EQ(protocol.decodeReply(source.sent.front().second).result, Reply::Result::Retry);
	EXPECT_TRUE(replicas.receiveMessage(source.sent.front().second, now));
	EXPECT_EQ(replicas.dispatchRequests(now + 999ms), 0u);
	EXPECT_EQ(replicas.dispatchRequests(now + 1s), 1u);
	source.failRequest = false;
	source.immediate = true;
	EXPECT_TRUE(source.receiveMessage(peer, replicas.sent.back()));
	(void)source.dispatch(now + 1s);
	ASSERT_EQ(source.sent.size(), 3u);
	EXPECT_TRUE(replicas.receiveMessage(source.sent[1].second, now + 1s));
	EXPECT_TRUE(replicas.receiveMessage(source.sent[2].second, now + 1s));
	EXPECT_EQ(replicas.requestStatus(object), Status::Ready);
}

TEST_F(NetworkTraitsTest, RequestTransportFailureUsesBoundedBackoff)
{
	RequestReplicas replicas;
	replicas.resetSession(session);
	replicas.requestObject(object, now);
	replicas.blocked = true;
	EXPECT_EQ(replicas.dispatchRequests(now), 0u);
	EXPECT_EQ(replicas.requestStatus(object), Status::Waiting);
	EXPECT_EQ(replicas.dispatchRequests(now + 999ms), 0u);
	replicas.blocked = false;
	replicas.throwSend = true;
	EXPECT_EQ(replicas.dispatchRequests(now + 1s), 0u);
	replicas.throwSend = false;
	EXPECT_EQ(replicas.dispatchRequests(now + 2999ms), 0u);
	EXPECT_EQ(replicas.dispatchRequests(now + 3s), 1u);
	const auto request = protocol.decodeRequest(replicas.sent.back());
	EXPECT_EQ(request.attempt, 3u);
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(Reply{request, Reply::Result::Rejected}), now + 3s));
	EXPECT_EQ(replicas.requestStatus(object), Status::Failed);
	EXPECT_EQ(replicas.dispatchRequests(now + 1h), 0u);
}

TEST_F(NetworkTraitsTest, ReadyRequiresReplicaAndCancelledRequestsIgnoreReplies)
{
	RequestReplicas replicas;
	replicas.resetSession(session);
	replicas.requestObject(object, now);
	EXPECT_EQ(replicas.dispatchRequests(now), 1u);
	const auto request = protocol.decodeRequest(replicas.sent.back());
	const auto ready = protocol.encode(Reply{request, Reply::Result::Ready});
	EXPECT_FALSE(replicas.receiveMessage(ready, now));
	EXPECT_EQ(replicas.requestStatus(object), Status::Pending);
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(update(4)), now));
	replicas.cancelRequest(object);
	EXPECT_FALSE(replicas.receiveMessage(ready, now));
	EXPECT_FALSE(replicas.requestStatus(object).has_value());
	EXPECT_EQ(replicas.objects.at(object).value, 4);
	replicas.requestObject(object, now);
	EXPECT_EQ(replicas.dispatchRequests(now), 1u);
	EXPECT_FALSE(replicas.receiveMessage(ready, now));
	replicas.resetSession(ID::generate());
	EXPECT_TRUE(replicas.objects.empty());
	EXPECT_FALSE(replicas.requestStatus(object).has_value());
}

TEST_F(NetworkTraitsTest, RequestTransportCannotResetOrMutateCollectionDuringSend)
{
	RequestReplicas replicas;
	replicas.resetSession(session);
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(update(4)), now));
	replicas.requestObject(object, now);
	replicas.onSend = [&] {
		replicas.resetSession(ID::generate());
	};
	EXPECT_EQ(replicas.dispatchRequests(now), 0u);
	EXPECT_EQ(replicas.objects.at(object).value, 4);
	replicas.onSend = {};
	EXPECT_EQ(replicas.dispatchRequests(now + 1s), 1u);
	EXPECT_EQ(protocol.decodeRequest(replicas.sent.back()).session, session);
}

TEST_F(NetworkTraitsTest, DisconnectClearsReplicasAndRequestsAndRequiresNewHandshake)
{
	RequestReplicas replicas;
	replicas.resetSession(session);
	const auto message = protocol.encode(update(4));
	EXPECT_TRUE(replicas.receiveMessage(message, now));
	replicas.requestObject(object, now);
	replicas.closeSession();
	EXPECT_TRUE(replicas.objects.empty());
	EXPECT_FALSE(replicas.requestStatus(object).has_value());
	EXPECT_FALSE(replicas.receiveMessage(message, now));
	EXPECT_EQ(replicas.dispatchRequests(now), 0u);
	EXPECT_THROW(replicas.requestObject(object, now), spk::Exception);
	EXPECT_NO_THROW(replicas.closeSession());
	session = ID::generate();
	replicas.resetSession(session);
	replicas.requestObject(object, now);
	EXPECT_EQ(replicas.dispatchRequests(now), 1u);
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(update(5)), now));
}
