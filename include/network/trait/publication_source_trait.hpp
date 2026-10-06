#pragma once

#include "container/thread_safe_set.hpp"
#include "network/replication/operation_guard.hpp"
#include "network/replication/protocol.hpp"
#include "network/server.hpp"
#include "publishable_trait.hpp"
#include <algorithm>
#include <deque>
#include <map>
#include <optional>
#include <set>

namespace spk::Network
{
	// Owner-thread only. Objects are observed, never owned. Send hooks use one ordered transport.
	template <typename State, typename Codec>
	class PublicationSourceTrait
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
			PublishableTrait<State> *instance = nullptr;
			std::weak_ptr<void> lifetime;
			std::uint64_t edition = 0, revision = 0;
			std::shared_ptr<const State> state;
		};
		struct Pending
		{
			Update<State> update;
			spk::Message::RequestID requestID = 0;
		};
		struct Peer
		{
			SessionID session = SessionID::generate();
			spk::Message::RequestID lastRequest = 0;
			std::map<ObjectID, std::uint64_t> tracking;
			std::map<ObjectID, Request> requests;
			std::map<ObjectID, Pending> pending;
			std::deque<ObjectID> order;
		};
		struct Connection
		{
			PeerID peer;
			SessionID token;
		};
		spk::Server *_server = nullptr;
		std::shared_ptr<spk::ThreadSafeSet<spk::ConnectionID>> _live;
		std::map<spk::ConnectionID, Connection> _connections;
		Configuration _configuration;
		Protocol<State, Codec> _protocol;
		std::map<ObjectID, Object> _objects;
		std::map<PeerID, Peer> _peers;
		std::deque<PeerID> _roundRobin;
		Sequence _sequence;
		Clock::time_point _nextPublication = Clock::time_point::min();
		bool _active = false, _requesting = false;
		spk::Server::ConnectionContract _connectionContract;
		spk::Server::DisconnectionContract _disconnectionContract;
		spk::Server::MessageDispatcher::Contract _messageContract;
		spk::Server::MessageDispatcher::TreatmentContract _treatmentContract;

		void _requireIdle() const
		{
			if (_active)
			{
				throw spk::Exception("Publication mutation during application hook");
			}
		}
		Peer &_peer(PeerID id)
		{
			auto found = _peers.find(id);
			if (found == _peers.end())
			{
				throw spk::Exception("Unknown replication peer");
			}
			return found->second;
		}
		Object &_object(ObjectID id)
		{
			auto found = _objects.find(id);
			if (found == _objects.end())
			{
				throw spk::Exception("Unknown replication object");
			}
			return found->second;
		}
		void _room(const Peer &peer, ObjectID id) const
		{
			if (!peer.pending.contains(id) && peer.pending.size() >= _configuration.maximumObjects)
			{
				throw spk::Exception("Replication queue full; drain or close peer");
			}
		}
		void _queue(Peer &peer, ObjectID id, Edit edit)
		{
			auto [found, inserted] = peer.pending.try_emplace(id);
			if (inserted)
			{
				peer.order.push_back(id);
			}
			const auto &object = _object(id);
			found->second.update = {peer.session, id, peer.tracking.at(id), object.revision, edit, edit == Edit::Set ? object.state : nullptr};
			if (edit != Edit::Set)
			{
				found->second.requestID = 0;
			}
		}
		void _notify(ObjectID id, Edit edit)
		{
			for (auto &[peerID, peer] : _peers)
			{
				if (peer.tracking.contains(id))
				{
					_queue(peer, id, edit);
				}
			}
		}
		void _checkFollowers(ObjectID id) const
		{
			for (const auto &[peerID, peer] : _peers)
			{
				if (peer.tracking.contains(id))
				{
					_room(peer, id);
				}
			}
		}
		void _publish(ObjectID id, State state)
		{
			if (id.isNull() || (!_objects.contains(id) && _objects.size() >= _configuration.maximumObjects))
			{
				throw spk::Exception("Invalid object identity or object limit reached");
			}
			_checkFollowers(id);
			auto snapshot = std::make_shared<const State>(std::move(state));
			const auto revision = _sequence.next();
			auto &object = _objects[id];
			object.state = std::move(snapshot);
			object.revision = revision;
			_notify(id, Edit::Set);
		}
		void _captureChanges()
		{
			for (auto &[id, object] : _objects)
			{
				if (object.lifetime.expired() || object.edition == object.instance->networkEdition())
				{
					continue;
				}
				const auto edition = object.instance->networkEdition();
				_publish(id, object.instance->buildNetworkState());
				object.edition = edition;
			}
		}
		void _follow(Peer &peer, ObjectID id, spk::Message::RequestID requestID = 0)
		{
			(void)_object(id);
			_room(peer, id);
			if (!peer.tracking.contains(id))
			{
				peer.tracking.emplace(id, _sequence.next());
			}
			_queue(peer, id, Edit::Set);
			if (requestID != 0)
			{
				peer.pending.at(id).requestID = requestID;
			}
		}
		Peer *_requestPeer(PeerID id, const Request &request)
		{
			auto found = _peers.find(id);
			if (found == _peers.end() || found->second.session != request.session)
			{
				return nullptr;
			}
			auto &peer = found->second;
			auto pending = peer.requests.find(request.object);
			return pending != peer.requests.end() && pending->second == request ? &peer : nullptr;
		}
		bool _dispatchOne(PeerID id, DispatchResult &result)
		{
			auto &peer = _peer(id);
			if (peer.order.empty())
			{
				return false;
			}
			const auto &pending = peer.pending.at(peer.order.front());
			try
			{
				if (_sendMessage(id, _protocol.encode(pending.update, pending.requestID)))
				{
					peer.pending.erase(peer.order.front());
					peer.order.pop_front();
					++result.sent;
					return true;
				}
			} catch (...)
			{
				++result.errors;
			}
			++result.blocked;
			return false;
		}

		void _synchronizeBinding()
		{
			_requireIdle();
			std::erase_if(_connections, [this](const auto &entry) {
				if (_live->contains(entry.first))
				{
					return false;
				}
				closePeer(entry.second.peer);
				return true;
			});
		}
		void _receiveHello(spk::ConnectionID id, const spk::Message &message)
		{
			const auto hello = _protocol.decodeHandshake(message);
			auto found = _connections.find(id);
			if (found != _connections.end() && (found->second.token != hello.token || !_peers.contains(found->second.peer)))
			{
				closePeer(found->second.peer);
				_connections.erase(found);
			}
			if (!_connections.contains(id))
			{
				const auto peer = PeerID::generate();
				(void)openPeer(peer);
				_connections.emplace(id, Connection{peer, hello.token});
			}
			const auto peer = _connections.at(id).peer;
			_server->sendTo(id, _protocol.encodeHandshake(hello.token, _peer(peer).session));
		}
		void _receiveBoundMessage(const spk::ReceivedMessage &received)
		{
			_synchronizeBinding();
			if (!_live->contains(received.emitter))
			{
				return;
			}
			if (_protocol.kind(received.message) == Protocol<State, Codec>::Kind::Hello)
			{
				_receiveHello(received.emitter, received.message);
			}
			else if (auto peer = peerID(received.emitter))
			{
				(void)receiveMessage(*peer, received.message);
			}
		}

	protected:
		// true means accepted by the ordered transport, not acknowledged remotely.
		[[nodiscard]] virtual bool _sendMessage(PeerID peer, const spk::Message &message)
		{
			if (!isBound())
			{
				throw spk::Exception("Publication source has no transport");
			}
			for (const auto &[connection, binding] : _connections)
			{
				if (binding.peer == peer && _live->contains(connection))
				{
					_server->sendTo(connection, message);
					return true;
				}
			}
			return false;
		}
		// May accept/reject immediately, or complete later on the owner thread.
		virtual void _requestObject(PeerID peer, const Request &request)
		{
			if (_objects.contains(request.object))
			{
				(void)acceptRequest(peer, request);
			}
			else
			{
				(void)rejectRequest(peer, request);
			}
		}

	public:
		explicit PublicationSourceTrait(spk::Message::Type type, Configuration configuration = {}, std::size_t maximumBytes = 2 * 1024 * 1024) :
			_configuration(configuration),
			_protocol(type, maximumBytes)
		{
			if (configuration.maximumObjects == 0 || configuration.maximumPeers == 0 || configuration.interval < Clock::duration::zero())
			{
				throw spk::Exception("Invalid publication configuration");
			}
		}
		virtual ~PublicationSourceTrait() = default;
		PublicationSourceTrait(const PublicationSourceTrait &) = delete;
		PublicationSourceTrait &operator=(const PublicationSourceTrait &) = delete;
		// Bind before starting the server. Treat messages and mutate this source on one owner thread.
		void bind(spk::Server &server)
		{
			_requireIdle();
			if (_server == &server && isBound())
			{
				return;
			}
			if (server.isRunning())
			{
				throw spk::Exception("Bind publication before starting the server");
			}
			unbind();
			auto live = std::make_shared<spk::ThreadSafeSet<spk::ConnectionID>>();
			auto connected = server.subscribeToConnection([live](spk::ConnectionID id) {
				(void)live->publish(id);
			});
			auto disconnected = server.subscribeToDisconnection([live](spk::ConnectionID id) {
				(void)live->erase(id);
			});
			auto messages = server.messageDispatcher().subscribeTo(_protocol.type(), [this](const auto &message) {
				_receiveBoundMessage(message);
			});
			auto treatment = server.messageDispatcher().subscribeToTreatment([this] {
				_synchronizeBinding();
			});
			_server = &server;
			_live = std::move(live);
			_connectionContract = std::move(connected);
			_disconnectionContract = std::move(disconnected);
			_messageContract = std::move(messages);
			_treatmentContract = std::move(treatment);
		}
		void unbind()
		{
			_requireIdle();
			_messageContract.resign();
			_treatmentContract.resign();
			_connectionContract.resign();
			_disconnectionContract.resign();
			for (const auto &[id, connection] : _connections)
			{
				closePeer(connection.peer);
			}
			_connections.clear();
			_live.reset();
			_server = nullptr;
		}
		[[nodiscard]] bool isBound() const noexcept
		{
			return _messageContract.isValid();
		}
		[[nodiscard]] std::optional<PeerID> peerID(spk::ConnectionID connection) const
		{
			auto found = _connections.find(connection);
			return found == _connections.end() || !_peers.contains(found->second.peer) ? std::nullopt : std::optional{found->second.peer};
		}

		void registerObject(ObjectID id, PublishableTrait<State> &instance)
		{
			OperationGuard guard(_active);
			auto found = _objects.find(id);
			if (found != _objects.end() && !found->second.lifetime.expired())
			{
				throw spk::Exception("Network object already registered");
			}
			const auto edition = instance.networkEdition();
			_publish(id, instance.buildNetworkState());
			auto &object = _object(id);
			object.instance = &instance;
			object.lifetime = instance._lifetime;
			object.edition = edition;
		}
		// Detachment and C++ destruction retain the last snapshot. Deletion is explicit.
		void unregisterObject(ObjectID id)
		{
			_requireIdle();
			if (auto found = _objects.find(id); found != _objects.end())
			{
				found->second.lifetime.reset();
				found->second.instance = nullptr;
			}
		}
		void destroyObject(ObjectID id)
		{
			_requireIdle();
			_checkFollowers(id);
			_notify(id, Edit::Destroy);
			for (auto &[peerID, peer] : _peers)
			{
				peer.tracking.erase(id);
				peer.requests.erase(id);
			}
			_objects.erase(id);
		}
		[[nodiscard]] SessionID openPeer(PeerID id)
		{
			_requireIdle();
			if (id.isNull() || _peers.contains(id) || _peers.size() >= _configuration.maximumPeers)
			{
				throw spk::Exception("Invalid, duplicate or excess replication peer");
			}
			auto [found, inserted] = _peers.try_emplace(id);
			_roundRobin.push_back(id);
			return found->second.session;
		}
		void closePeer(PeerID id)
		{
			_requireIdle();
			_peers.erase(id);
			std::erase(_roundRobin, id);
		}
		void follow(PeerID peer, ObjectID id)
		{
			_requireIdle();
			if (!_peer(peer).tracking.contains(id))
			{
				_follow(_peer(peer), id);
			}
		}
		void forget(PeerID peerID, ObjectID id)
		{
			_requireIdle();
			auto &peer = _peer(peerID);
			if (peer.tracking.contains(id))
			{
				_room(peer, id);
				_queue(peer, id, Edit::Forget);
				peer.tracking.erase(id);
			}
			peer.requests.erase(id);
		}
		[[nodiscard]] bool receiveMessage(PeerID id, const spk::Message &message)
		{
			_requireIdle();
			OperationGuard guard(_requesting);
			const auto request = _protocol.decodeRequest(message);
			auto found = _peers.find(id);
			if (found == _peers.end() || found->second.session != request.session || request.id <= found->second.lastRequest)
			{
				return false;
			}
			auto &peer = found->second;
			if (!peer.requests.contains(request.object) && peer.requests.size() >= _configuration.maximumObjects)
			{
				throw spk::Exception("Pending request limit reached");
			}
			peer.requests.insert_or_assign(request.object, request);
			peer.lastRequest = request.id;
			_requestObject(id, request);
			return true;
		}
		[[nodiscard]] bool acceptRequest(PeerID id, const Request &request)
		{
			_requireIdle();
			auto *peer = _requestPeer(id, request);
			if (peer == nullptr)
			{
				return false;
			}
			_follow(*peer, request.object, request.id);
			peer->requests.erase(request.object);
			return true;
		}
		[[nodiscard]] bool fulfillRequest(PeerID id, const Request &request, State state)
		{
			_requireIdle();
			if (_requestPeer(id, request) == nullptr)
			{
				return false;
			}
			_room(_peer(id), request.object);
			_publish(request.object, std::move(state));
			return acceptRequest(id, request);
		}
		// A rejected/throwing send leaves the request pending for an explicit retry.
		[[nodiscard]] bool rejectRequest(PeerID id, const Request &request)
		{
			OperationGuard guard(_active);
			auto *peer = _requestPeer(id, request);
			if (peer == nullptr || !_sendMessage(id, _protocol.encode(request, Protocol<State, Codec>::Kind::Rejected)))
			{
				return false;
			}
			peer->requests.erase(request.object);
			return true;
		}
		DispatchResult dispatch(Clock::time_point now, std::size_t maximumAttempts = 64)
		{
			OperationGuard guard(_active);
			DispatchResult result;
			if (now < _nextPublication || maximumAttempts == 0 || _peers.empty())
			{
				return result;
			}
			_captureChanges();
			_nextPublication = now + _configuration.interval;
			std::set<PeerID> skipped;
			for (std::size_t count = 0; count < maximumAttempts && skipped.size() < _peers.size(); ++count)
			{
				const auto id = _roundRobin.front();
				_roundRobin.pop_front();
				_roundRobin.push_back(id);
				if (!skipped.contains(id) && !_dispatchOne(id, result))
				{
					skipped.insert(id);
				}
			}
			return result;
		}
	};
}
