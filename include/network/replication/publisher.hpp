#pragma once

#include "sequence.hpp"
#include "update.hpp"
#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <utility>

namespace spk::Network
{
	// One owner thread. Send callbacks must not reenter this publisher.
	// State must have no mutable aliases into application objects.
	template <typename State>
	class Publisher final
	{
	public:
		struct Configuration
		{
			std::size_t maximumObjects = 16384, maximumPeers = 256;
			Clock::duration interval = std::chrono::milliseconds(50);
		};
		struct DispatchResult
		{
			std::size_t sent = 0, blocked = 0, errors = 0;
		};

	private:
		struct Object
		{
			std::shared_ptr<const State> state;
			Sequence revision{1};
			std::set<PeerID> followers;
		};
		struct Peer
		{
			SessionID session;
			std::map<ObjectID, std::uint64_t> tracking;
			std::deque<ObjectID> order;
			std::map<ObjectID, Update<State>> pending;
		};
		Configuration _configuration;
		std::map<ObjectID, Object> _objects;
		std::map<PeerID, Peer> _peers;
		std::deque<PeerID> _roundRobin;
		Sequence _tracking;
		Clock::time_point _nextPublication = Clock::time_point::min();
		bool _dispatching = false;
		void _check() const;
		Peer &_peer(PeerID id);
		Object &_object(ObjectID id);
		void _queue(Peer &peer, Update<State> update);
		void _state(Peer &peer, ObjectID id, const Object &object);
		void _room(const Peer &peer, ObjectID id) const;

	public:
		explicit Publisher(Configuration configuration = {});
		Publisher(const Publisher &) = delete;
		Publisher &operator=(const Publisher &) = delete;
		// Return this fresh session through the trusted connection handshake.
		[[nodiscard]] SessionID open(PeerID id);
		[[nodiscard]] std::optional<SessionID> session(PeerID id) const;
		// True means the latest snapshot has been accepted by the ordered transport,
		// not acknowledged by the remote application. A Ready reply may now follow it.
		[[nodiscard]] bool synchronized(PeerID peerID, ObjectID objectID) const;
		void close(PeerID id);
		void publish(ObjectID id, State state);
		// Same operation after an accepted client request or a server interest decision.
		void follow(PeerID peerID, ObjectID id);
		void forget(PeerID peerID, ObjectID id);
		void destroy(ObjectID id);
		// Sender(peer, update) returns true ONLY when the ordered transport accepts it.
		// false or exception retains this update, but other peers still get a turn.
		template <typename Sender>
		DispatchResult dispatch(Clock::time_point now, std::size_t maximumAttempts, Sender &&send);
	};
} // namespace spk::Network

#include "publisher.tpp"
