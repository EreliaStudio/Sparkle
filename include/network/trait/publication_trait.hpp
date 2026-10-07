#pragma once
#include "network/replication/operation_guard.hpp"
#include "object_interest_trait.hpp"
#include "peer_session_trait.hpp"
#include "publication_queue_trait.hpp"
#include "published_object_collection_trait.hpp"
namespace spk::Network
{
	template <typename State>
	class PublicationTrait : protected PublishedObjectCollectionTrait<State>, protected PeerSessionTrait, protected ObjectInterestTrait, protected PublicationQueueTrait<State>
	{
		using Objects = PublishedObjectCollectionTrait<State>;
		using Queue = PublicationQueueTrait<State>;
		bool _active = false;
		void _beforeSnapshot(ObjectID id) override
		{
			_checkFollowers(id);
		}
		void _onSnapshot(ObjectID id) override
		{
			_notify(id, Edit::Set);
		}
		void _capturePublicationChanges() override
		{
			this->_captureChanges();
		}
		void _notify(ObjectID id, Edit edit)
		{
			for (const auto peer : _followers(id))
			{
				_queueObject(peer, id, edit);
			}
		}
		void _checkFollowers(ObjectID id) const
		{
			for (const auto peer : _followers(id))
			{
				this->_room(peer, id);
			}
		}
		void _queueObject(PeerID peer, ObjectID id, Edit edit, spk::Message::RequestID requestID = 0)
		{
			const auto &object = this->_snapshot(id);
			this->_queue(peer, {_peerSession(peer), id, _trackingID(peer, id), object.revision, edit, edit == Edit::Set ? object.state : nullptr}, requestID);
		}

	protected:
		void _requirePublicationIdle() const
		{
			if (_active)
			{
				throw spk::Exception("Publication mutation during application hook");
			}
		}
		[[nodiscard]] bool &_publicationOperation()
		{
			return _active;
		}
		using Objects::_hasSnapshot;
		using Objects::_publish;
		using PeerSessionTrait::_findSession;
		using PeerSessionTrait::_peerSession;
		using Queue::_room;
		virtual void _onPeerClosed(PeerID)
		{
		}
		virtual void _onObjectForgotten(PeerID, ObjectID)
		{
		}
		virtual void _onObjectDestroyed(ObjectID)
		{
		}
		void _follow(PeerID peer, ObjectID id, spk::Message::RequestID requestID = 0)
		{
			(void)_peerSession(peer);
			(void)this->_snapshot(id);
			this->_room(peer, id);
			(void)_track(peer, id);
			_queueObject(peer, id, Edit::Set, requestID);
		}

	public:
		struct Configuration
		{
			std::size_t maximumObjects = 16384, maximumPeers = 256;
			Clock::duration interval = std::chrono::milliseconds(50);
		};
		using DispatchResult = typename Queue::DispatchResult;
		explicit PublicationTrait(Configuration configuration = {}) :
			Objects(configuration.maximumObjects),
			PeerSessionTrait(configuration.maximumPeers),
			Queue(configuration.maximumObjects, configuration.interval)
		{
		}
		virtual ~PublicationTrait() = default;
		void registerObject(ObjectID id, PublishableTrait<State> &instance)
		{
			OperationGuard guard(_active);
			this->_registerObject(id, instance);
		}
		void unregisterObject(ObjectID id)
		{
			_requirePublicationIdle();
			this->_unregisterObject(id);
		}
		void destroyObject(ObjectID id)
		{
			OperationGuard guard(_active);
			_checkFollowers(id);
			_notify(id, Edit::Destroy);
			_forgetObjectInterest(id);
			_onObjectDestroyed(id);
			this->_eraseSnapshot(id);
		}
		[[nodiscard]] SessionID openPeer(PeerID peer)
		{
			OperationGuard guard(_active);
			const auto session = _openSession(peer);
			try
			{
				this->_openQueue(peer);
			} catch (...)
			{
				_closeSession(peer);
				throw;
			}
			return session;
		}
		void closePeer(PeerID peer)
		{
			OperationGuard guard(_active);
			_onPeerClosed(peer);
			this->_closeQueue(peer);
			_forgetPeerInterest(peer);
			_closeSession(peer);
		}
		void follow(PeerID peer, ObjectID id)
		{
			OperationGuard guard(_active);
			(void)_peerSession(peer);
			if (!_follows(peer, id))
			{
				_follow(peer, id);
			}
		}
		void forget(PeerID peer, ObjectID id)
		{
			OperationGuard guard(_active);
			(void)_peerSession(peer);
			if (_follows(peer, id))
			{
				_queueObject(peer, id, Edit::Forget);
				_forgetInterest(peer, id);
			}
			_onObjectForgotten(peer, id);
		}
		DispatchResult dispatch(Clock::time_point now, std::size_t maximumAttempts = 64)
		{
			OperationGuard guard(_active);
			return this->_dispatchPublication(now, maximumAttempts);
		}
	};
}
