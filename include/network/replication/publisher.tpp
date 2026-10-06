#pragma once

#include "exception.hpp"

namespace spk::Network
{
	template <typename State>
	void Publisher<State>::_check() const
	{
		if (_dispatching)
		{
			throw spk::Exception("Publisher mutation during send callback");
		}
	}

	template <typename State>
	typename Publisher<State>::Peer &Publisher<State>::_peer(PeerID id)
	{
		auto found = _peers.find(id);
		if (found == _peers.end())
		{
			throw spk::Exception("Unknown replication peer");
		}
		return found->second;
	}

	template <typename State>
	typename Publisher<State>::Object &Publisher<State>::_object(ObjectID id)
	{
		auto found = _objects.find(id);
		if (found == _objects.end())
		{
			throw spk::Exception("Unknown replication object");
		}
		return found->second;
	}

	template <typename State>
	void Publisher<State>::_queue(Peer &peer, Update<State> update)
	{
		if (!peer.pending.contains(update.object))
		{
			if (peer.pending.size() == _configuration.maximumObjects)
			{
				throw spk::Exception("Replication queue full; drain or close the peer");
			}
			peer.order.push_back(update.object);
		}
		peer.pending.insert_or_assign(update.object, std::move(update));
	}

	template <typename State>
	void Publisher<State>::_state(Peer &peer, ObjectID id, const Object &object)
	{
		_queue(peer, {peer.session, id, peer.tracking.at(id), object.revision.value(), Edit::Set, object.state});
	}

	template <typename State>
	void Publisher<State>::_room(const Peer &peer, ObjectID id) const
	{
		if (!peer.pending.contains(id) &&
			peer.pending.size() >= _configuration.maximumObjects)
		{
			throw spk::Exception("Replication queue full; drain or close the peer");
		}
	}

	template <typename State>
	Publisher<State>::Publisher(Configuration configuration) :
		_configuration(configuration)
	{
		if (configuration.maximumObjects == 0 || configuration.maximumPeers == 0 ||
			configuration.interval < Clock::duration::zero())
		{
			throw spk::Exception("Invalid publisher configuration");
		}
	}

	template <typename State>
	SessionID Publisher<State>::open(PeerID id)
	{
		_check();
		if (id.isNull())
		{
			throw spk::Exception("Null network identity");
		}
		if (_peers.contains(id))
		{
			throw spk::Exception("Close the previous peer session first");
		}
		if (_peers.size() == _configuration.maximumPeers)
		{
			throw spk::Exception("Peer limit reached");
		}
		const auto session = SessionID::generate();
		_peers.emplace(id, Peer{session, {}, {}, {}});
		_roundRobin.push_back(id);
		return session;
	}

	template <typename State>
	std::optional<SessionID> Publisher<State>::session(PeerID id) const
	{
		const auto found = _peers.find(id);
		if (found == _peers.end())
		{
			return std::nullopt;
		}
		return found->second.session;
	}

	template <typename State>
	bool Publisher<State>::synchronized(PeerID peerID, ObjectID objectID) const
	{
		const auto found = _peers.find(peerID);
		return found != _peers.end() && found->second.tracking.contains(objectID) &&
			   !found->second.pending.contains(objectID);
	}

	template <typename State>
	void Publisher<State>::close(PeerID id)
	{
		_check();
		if (!_peers.contains(id))
		{
			return;
		}
		for (const auto &[key, tracking] : _peer(id).tracking)
		{
			_object(key).followers.erase(id);
		}
		_peers.erase(id);
		std::erase(_roundRobin, id);
	}

	template <typename State>
	void Publisher<State>::publish(ObjectID id, State state)
	{
		_check();
		if (id.isNull())
		{
			throw spk::Exception("Null network identity");
		}
		auto found = _objects.find(id);
		if (found == _objects.end())
		{
			if (_objects.size() == _configuration.maximumObjects)
			{
				throw spk::Exception("Object limit reached");
			}
			_objects.emplace(
				id,
				Object{std::make_shared<const State>(std::move(state)), Sequence{1}, {}});
			return;
		}
		auto &object = found->second;
		for (auto peer : object.followers)
		{
			_room(_peer(peer), id);
		}
		auto snapshot = std::make_shared<const State>(std::move(state));
		(void)object.revision.next();
		object.state = std::move(snapshot);
		for (auto peer : object.followers)
		{
			_state(_peer(peer), id, object);
		}
	}

	template <typename State>
	void Publisher<State>::follow(PeerID peerID, ObjectID id)
	{
		_check();
		auto &peer = _peer(peerID);
		auto &object = _object(id);
		if (peer.tracking.contains(id))
		{
			return;
		}
		_room(peer, id);
		const auto tracking = _tracking.next();
		peer.tracking.emplace(id, tracking);
		object.followers.insert(peerID);
		_state(peer, id, object);
	}

	template <typename State>
	void Publisher<State>::forget(PeerID peerID, ObjectID id)
	{
		_check();
		auto &peer = _peer(peerID);
		const auto found = peer.tracking.find(id);
		if (found == peer.tracking.end())
		{
			return;
		}
		auto &object = _object(id);
		_room(peer, id);
		_queue(peer, {peer.session, id, found->second, object.revision.value(), Edit::Forget, nullptr});
		peer.tracking.erase(found);
		object.followers.erase(peerID);
	}

	template <typename State>
	void Publisher<State>::destroy(ObjectID id)
	{
		_check();
		auto found = _objects.find(id);
		if (found == _objects.end())
		{
			return;
		}
		for (auto peer : found->second.followers)
		{
			_room(_peer(peer), id);
		}
		for (auto peerID : found->second.followers)
		{
			auto &peer = _peer(peerID);
			_queue(peer, {peer.session, id, peer.tracking.at(id), found->second.revision.value(), Edit::Destroy, nullptr});
			peer.tracking.erase(id);
		}
		_objects.erase(found);
	}

	template <typename State>
	template <typename Sender>
	typename Publisher<State>::DispatchResult Publisher<State>::dispatch(Clock::time_point now, std::size_t maximumAttempts, Sender &&send)
	{
		_check();
		DispatchResult result;
		if (now < _nextPublication || _roundRobin.empty() || maximumAttempts == 0)
		{
			return result;
		}
		_nextPublication = now + _configuration.interval;
		_dispatching = true;
		struct Guard
		{
			bool &flag;
			~Guard()
			{
				flag = false;
			}
		} guard{_dispatching};
		std::set<PeerID> skipped;
		for (std::size_t count = 0;
			 count < maximumAttempts && skipped.size() < _peers.size();
			 ++count)
		{
			const auto id = _roundRobin.front();
			_roundRobin.pop_front();
			_roundRobin.push_back(id);
			auto &peer = _peer(id);
			if (skipped.contains(id))
			{
				continue;
			}
			if (peer.order.empty())
			{
				skipped.insert(id);
				continue;
			}
			bool accepted = false;
			try
			{
				accepted = std::invoke(
					send, id, std::as_const(peer.pending.at(peer.order.front())));
			} catch (...)
			{
				++result.errors;
			}
			if (!accepted)
			{
				++result.blocked;
				skipped.insert(id);
				continue;
			}
			peer.pending.erase(peer.order.front());
			peer.order.pop_front();
			++result.sent;
		}
		return result;
	}
}
