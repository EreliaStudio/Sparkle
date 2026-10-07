#include "network/network_traits_test.hpp"
#include <type_traits>
using namespace std::chrono_literals;

class NetworkTraitCompositionTest : public NetworkTraitsTest
{
protected:
	class Snapshots : public spk::Network::PublishedObjectCollectionTrait
	{
	public:
		using PublishedObjectCollectionTrait::_captureChanges;
		using PublishedObjectCollectionTrait::_registerObject;
		using PublishedObjectCollectionTrait::_snapshot;
		using PublishedObjectCollectionTrait::_unregisterObject;
		using PublishedObjectCollectionTrait::PublishedObjectCollectionTrait;
	};
	class Interests : public spk::Network::ObjectInterestTrait, public spk::Network::PeerSessionTrait
	{
	public:
		Interests() :
			PeerSessionTrait(1)
		{
		}
		using ObjectInterestTrait::_follows;
		using ObjectInterestTrait::_forgetInterest;
		using ObjectInterestTrait::_track;
		using PeerSessionTrait::_closeSession;
		using PeerSessionTrait::_findSession;
		using PeerSessionTrait::_openSession;
	};
	class Queue : public spk::Network::PublicationQueueTrait
	{
		bool _sendUpdate(ID peer, const Update &update, spk::Message::RequestID request) override
		{
			if (blocked.contains(peer))
			{
				return false;
			}
			sent.emplace_back(update, request);
			return true;
		}

	public:
		Queue() :
			PublicationQueueTrait(2, 50ms)
		{
		}
		using PublicationQueueTrait::_dispatchPublication;
		using PublicationQueueTrait::_openQueue;
		using PublicationQueueTrait::_queue;
		std::set<ID> blocked;
		std::vector<std::pair<Update, spk::Message::RequestID>> sent;
	};
	class Handler : public spk::Network::ObjectRequestHandlerTrait, protected spk::Network::PeerSessionTrait
	{
		std::optional<ID> _requestSession(ID peer) const override
		{
			return _findSession(peer);
		}
		void _requestObject(ID, const Request &request) override
		{
			received.push_back(request);
		}

	public:
		Handler() :
			ObjectRequestHandlerTrait(1)
		{
		}
		using ObjectRequestHandlerTrait::_completeRequest;
		using ObjectRequestHandlerTrait::_isCurrentRequest;
		using PeerSessionTrait::_openSession;
		void close(ID peer)
		{
			_cancelPeerRequests(peer);
			_closeSession(peer);
		}
		std::vector<Request> received;
	};
	class Requester : public spk::Network::ObjectRequesterTrait
	{
		ID _requestSession() const override
		{
			return session;
		}
		bool _sendObjectRequest(const Request &request) override
		{
			sent.push_back(request);
			if (throwSend)
			{
				throw spk::Exception("send");
			}
			return !blocked;
		}

	public:
		Requester() :
			ObjectRequesterTrait(1)
		{
		}
		using ObjectRequesterTrait::_rejectAcquisition;
		ID session = ID::generate();
		bool blocked = false, throwSend = false;
		std::vector<Request> sent;
	};
	class History : public spk::Network::ReplicaHistoryTrait
	{
	public:
		History() :
			ReplicaHistoryTrait(1)
		{
		}
		using ReplicaHistoryTrait::_applyTracked;
		using ReplicaHistoryTrait::_tracksActive;
	};
	class PayloadReplicas : public spk::Network::ReplicaTrait
	{
		spk::Network::ReplicableTrait *_findReplica(ID id) override
		{
			auto found = objects.find(id);
			return found == objects.end() ? nullptr : &found->second;
		}
		spk::Network::ReplicableTrait &_createReplica(ID id) override
		{
			return objects[id];
		}
		void _removeReplica(ID id) override
		{
			objects.erase(id);
		}

	public:
		std::map<ID, Object> objects;
	};
	class PayloadSource : public spk::Network::PublicationTrait
	{
		bool _sendUpdate(ID, const Update &update, spk::Message::RequestID) override
		{
			if (onSend)
			{
				onSend();
			}
			(void)target.receiveUpdate(update);
			return true;
		}

	public:
		explicit PayloadSource(PayloadReplicas &replicas) :
			PublicationTrait({.interval = 0ms}),
			target(replicas)
		{
		}
		PayloadReplicas &target;
		std::function<void()> onSend;
	};
};

TEST_F(NetworkTraitCompositionTest, SnapshotObservationKeepsIndependentVersionsAndDetachedState)
{
	Snapshots first, second;
	{
		Object entity;
		first._registerObject(object, entity);
		second._registerObject(object, entity);
		entity.change(8);
		first._captureChanges();
		EXPECT_EQ(first._snapshot(object).payload->reader().get<int>(), 8);
		EXPECT_EQ(second._snapshot(object).payload->reader().get<int>(), 0);
		second._captureChanges();
		EXPECT_EQ(second._snapshot(object).payload->reader().get<int>(), 8);
		first._unregisterObject(object);
		entity.change(9);
		first._captureChanges();
		second._captureChanges();
	}
	EXPECT_NO_THROW(first._captureChanges());
	EXPECT_NO_THROW(second._captureChanges());
	EXPECT_EQ(first._snapshot(object).payload->reader().get<int>(), 8);
	EXPECT_EQ(second._snapshot(object).payload->reader().get<int>(), 9);
}

TEST_F(NetworkTraitCompositionTest, PeerSessionsAndInterestGenerationsHaveIndependentLifetimes)
{
	Interests interests;
	const auto session = interests._openSession(peer);
	const auto tracking = interests._track(peer, object);
	EXPECT_EQ(interests._track(peer, object), tracking);
	EXPECT_THROW((void)interests._openSession(ID::generate()), spk::Exception);
	interests._forgetInterest(peer, object);
	EXPECT_FALSE(interests._follows(peer, object));
	EXPECT_GT(interests._track(peer, object), tracking);
	interests._closeSession(peer);
	EXPECT_FALSE(interests._findSession(peer));
	EXPECT_NE(interests._openSession(peer), session);
}

TEST_F(NetworkTraitCompositionTest, QueueCoalescesWithoutLosingCorrelationOrFifoPosition)
{
	Queue queue;
	queue._openQueue(peer);
	auto first = update(1);
	auto second = update(2);
	second.object = ID::generate();
	queue._queue(peer, first, 7);
	queue._queue(peer, second);
	first.payload = payload(3);
	first.revision = 2;
	queue._queue(peer, first);
	EXPECT_EQ(queue._dispatchPublication(now, 1).sent, 1u);
	ASSERT_EQ(queue.sent.size(), 1u);
	EXPECT_EQ(queue.sent[0].first.payload->reader().get<int>(), 3);
	EXPECT_EQ(queue.sent[0].second, 7u);
	EXPECT_EQ(queue._dispatchPublication(now + 49ms, 1).sent, 0u);
	EXPECT_EQ(queue._dispatchPublication(now + 50ms, 1).sent, 1u);
	EXPECT_EQ(queue.sent[1].first.object, second.object);
	first.edit = Edit::Forget;
	first.payload.reset();
	queue._queue(peer, update(4), 9);
	queue._queue(peer, first);
	EXPECT_EQ(queue._dispatchPublication(now + 100ms, 1).sent, 1u);
	EXPECT_EQ(queue.sent.back().second, 0u);
}

TEST_F(NetworkTraitCompositionTest, HandlerRejectsSupersededResultsAndRetainsDuplicateHistory)
{
	Handler handler;
	const Request first{handler._openSession(peer), object, 1};
	Request next = first;
	next.id = 2;
	EXPECT_TRUE(handler.receiveRequest(peer, first));
	EXPECT_TRUE(handler.receiveRequest(peer, next));
	EXPECT_FALSE(handler._isCurrentRequest(peer, first));
	EXPECT_TRUE(handler._isCurrentRequest(peer, next));
	handler._completeRequest(peer, object);
	EXPECT_FALSE(handler.receiveRequest(peer, next));
	handler.close(peer);
	EXPECT_FALSE(handler._isCurrentRequest(peer, next));
	const Request fresh{handler._openSession(peer), object, 1};
	EXPECT_FALSE(handler.receiveRequest(peer, next));
	EXPECT_TRUE(handler.receiveRequest(peer, fresh));
}

TEST_F(NetworkTraitCompositionTest, RequesterPreservesPreviousCorrelationWhenRetrySendFails)
{
	Requester requester;
	ASSERT_TRUE(requester.requestObject(object));
	const auto original = requester.sent.back();
	requester.throwSend = true;
	EXPECT_THROW((void)requester.requestObject(object), spk::Exception);
	EXPECT_TRUE(requester._rejectAcquisition(original));
	EXPECT_EQ(requester.requestStatus(object), Status::Failed);
	requester.throwSend = false;
	requester.blocked = true;
	EXPECT_FALSE(requester.requestObject(object));
	EXPECT_EQ(requester.requestStatus(object), Status::Failed);
	requester.blocked = false;
	EXPECT_TRUE(requester.requestObject(object));
	EXPECT_FALSE(requester._rejectAcquisition(original));
	EXPECT_EQ(requester.requestStatus(object), Status::Pending);
}

TEST_F(NetworkTraitCompositionTest, HistoryCommitsOnlyAfterApplicationAndRetainsTombstones)
{
	History history;
	EXPECT_THROW((void)history._applyTracked(object, 1, 1, Edit::Set, [] {
		throw spk::Exception("apply");
	}),
				 spk::Exception);
	int applied = 0;
	auto apply = [&] {
		++applied;
	};
	ASSERT_TRUE(history._applyTracked(object, 1, 1, Edit::Set, apply));
	EXPECT_FALSE(history._applyTracked(object, 1, 1, Edit::Set, apply));
	ASSERT_TRUE(history._applyTracked(object, 1, 1, Edit::Forget, apply));
	EXPECT_FALSE(history._tracksActive(object, 1));
	EXPECT_FALSE(history._applyTracked(object, 1, 99, Edit::Set, apply));
	EXPECT_THROW((void)history._applyTracked(ID::generate(), 1, 1, Edit::Set, apply), spk::Exception);
	EXPECT_TRUE(history._applyTracked(object, 2, 1, Edit::Set, apply));
	EXPECT_EQ(applied, 3);
}

TEST_F(NetworkTraitCompositionTest, PayloadPublicationAndReplicationWorkWithoutTransportOrCodec)
{
	PayloadReplicas replicas;
	PayloadSource source(replicas);
	Object entity;
	replicas.resetSession(source.openPeer(peer));
	source.registerObject(object, entity);
	source.follow(peer, object);
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	entity.change(12);
	source.onSend = [&] {
		source.closePeer(peer);
	};
	EXPECT_EQ(source.dispatch(now).errors, 1u);
	source.onSend = {};
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	ASSERT_TRUE(replicas.objects.contains(object));
	EXPECT_EQ(replicas.objects.at(object).value, 12);
	source.forget(peer, object);
	EXPECT_EQ(source.dispatch(now).sent, 1u);
	EXPECT_TRUE(replicas.objects.empty());
}

TEST_F(NetworkTraitCompositionTest, RequesterBaseCannotMutateDuringReplicaApplication)
{
	RequestReplicas replicas;
	replicas.resetSession(session);
	spk::Network::ObjectRequesterTrait &requester = replicas;
	replicas.onApply = [&] {
		(void)requester.requestObject(object);
	};
	EXPECT_THROW((void)replicas.receiveMessage(protocol.encode(update(1))), spk::Exception);
	EXPECT_TRUE(replicas.sent.empty());
	EXPECT_FALSE(requester.requestStatus(object));
	replicas.onApply = {};
	EXPECT_TRUE(replicas.receiveMessage(protocol.encode(update(1))));
}
