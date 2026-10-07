#include "network/trait/publication_trait.hpp"

namespace spk::Network
{
	PublicationTrait::PublicationTrait() :
		PublicationTrait(Configuration{})
	{
	}
	void PublicationTrait::_beforeSnapshot(ObjectID id)
	{
		_checkFollowers(id);
	}
	void PublicationTrait::_onSnapshot(ObjectID id)
	{
		_notify(id, Edit::Set);
	}
	void PublicationTrait::_capturePublicationChanges()
	{
		_captureChanges();
	}
	void PublicationTrait::_notify(ObjectID id, Edit edit)
	{
		for (const auto peer : _followers(id))
		{
			_queueObject(peer, id, edit);
		}
	}
	void PublicationTrait::_checkFollowers(ObjectID id) const
	{
		for (const auto peer : _followers(id))
		{
			_room(peer, id);
		}
	}
	void PublicationTrait::_queueObject(PeerID peer, ObjectID id, Edit edit, spk::Message::RequestID requestID)
	{
		const auto &object = _snapshot(id);
		_queue(peer, {_peerSession(peer), id, _trackingID(peer, id), object.revision, edit, edit == Edit::Set ? object.payload : std::nullopt}, requestID);
	}
	void PublicationTrait::_requirePublicationIdle() const
	{
		if (_active)
		{
			throw spk::Exception("Publication mutation during application hook");
		}
	}
	bool &PublicationTrait::_publicationOperation()
	{
		return _active;
	}
	void PublicationTrait::_onPeerClosed(PeerID)
	{
	}
	void PublicationTrait::_onObjectForgotten(PeerID, ObjectID)
	{
	}
	void PublicationTrait::_onObjectDestroyed(ObjectID)
	{
	}
	void PublicationTrait::_follow(PeerID peer, ObjectID id, spk::Message::RequestID requestID)
	{
		(void)_peerSession(peer);
		(void)_snapshot(id);
		_room(peer, id);
		(void)_track(peer, id);
		_queueObject(peer, id, Edit::Set, requestID);
	}
	PublicationTrait::PublicationTrait(Configuration configuration) :
		Objects(configuration.maximumObjects),
		PeerSessionTrait(configuration.maximumPeers),
		Queue(configuration.maximumObjects, configuration.interval)
	{
	}
	void PublicationTrait::registerObject(ObjectID id, PublishableTrait &instance)
	{
		OperationGuard guard(_active);
		_registerObject(id, instance);
	}
	void PublicationTrait::unregisterObject(ObjectID id)
	{
		_requirePublicationIdle();
		_unregisterObject(id);
	}
	void PublicationTrait::destroyObject(ObjectID id)
	{
		OperationGuard guard(_active);
		_checkFollowers(id);
		_notify(id, Edit::Destroy);
		_forgetObjectInterest(id);
		_onObjectDestroyed(id);
		_eraseSnapshot(id);
	}
	SessionID PublicationTrait::openPeer(PeerID peer)
	{
		OperationGuard guard(_active);
		const auto session = _openSession(peer);
		try
		{
			_openQueue(peer);
		} catch (...)
		{
			_closeSession(peer);
			throw;
		}
		return session;
	}
	void PublicationTrait::closePeer(PeerID peer)
	{
		OperationGuard guard(_active);
		_onPeerClosed(peer);
		_closeQueue(peer);
		_forgetPeerInterest(peer);
		_closeSession(peer);
	}
	void PublicationTrait::follow(PeerID peer, ObjectID id)
	{
		OperationGuard guard(_active);
		(void)_peerSession(peer);
		if (!_follows(peer, id))
		{
			_follow(peer, id);
		}
	}
	void PublicationTrait::forget(PeerID peer, ObjectID id)
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
	PublicationTrait::DispatchResult PublicationTrait::dispatch(Clock::time_point now, std::size_t maximumAttempts)
	{
		OperationGuard guard(_active);
		return _dispatchPublication(now, maximumAttempts);
	}
}
