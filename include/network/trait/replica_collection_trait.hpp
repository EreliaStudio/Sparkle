#pragma once

#include "network/replication/protocol.hpp"
#include "network/replication/sequence.hpp"
#include "replicable_trait.hpp"
#include <map>
#include <optional>

namespace spk::Network
{
	// Owner-thread only. The application owns replica storage; the server controls its contents.
	template <typename State, typename Codec>
	class ReplicaCollectionTrait
	{
	public:
		enum class RequestStatus
		{
			Pending,
			Ready,
			Failed
		};

	private:
		struct Tracking
		{
			std::uint64_t identity = 0, revision = 0;
			bool active = false;
		};
		struct Acquisition
		{
			spk::Message::RequestID id;
			RequestStatus status = RequestStatus::Pending;
		};
		Protocol<State, Codec> _protocol;
		SessionID _session;
		std::size_t _maximumTracked;
		std::map<ObjectID, Tracking> _tracking;
		std::map<ObjectID, Acquisition> _requests;
		Sequence _sequence;
		bool _active = false;
		void _clear()
		{
			for (const auto &[id, tracking] : _tracking)
			{
				if (tracking.active)
				{
					_removeReplica(id);
				}
			}
			_tracking.clear();
			_requests.clear();
		}
		void _apply(const Update<State> &update)
		{
			if (update.edit != Edit::Set)
			{
				_removeReplica(update.object);
				_requests.erase(update.object);
				return;
			}
			auto *replica = _findReplica(update.object);
			if (replica != nullptr)
			{
				replica->applyNetworkState(*update.state);
				return;
			}
			auto &created = _createReplica(update.object);
			try
			{
				created.applyNetworkState(*update.state);
			} catch (...)
			{
				_removeReplica(update.object);
				throw;
			}
		}
		[[nodiscard]] bool _receiveUpdate(const spk::Message &message)
		{
			const auto update = _protocol.decodeUpdate(message);
			if (update.session != _session || _session.isNull())
			{
				return false;
			}
			// Correlation completes acquisition; it never overrides authoritative state ordering.
			const bool accepted = _accept(update);
			auto request = _requests.find(update.object);
			auto tracked = _tracking.find(update.object);
			if (request != _requests.end() && request->second.status == RequestStatus::Pending && request->second.id == message.requestID() &&
				tracked != _tracking.end() && tracked->second.active && tracked->second.identity == update.tracking && update.edit == Edit::Set)
			{
				request->second.status = RequestStatus::Ready;
				return true;
			}
			return accepted;
		}
		[[nodiscard]] static bool _obsolete(const Tracking &tracked, const Update<State> &update)
		{
			if (update.tracking != tracked.identity)
			{
				return update.tracking < tracked.identity;
			}
			return !tracked.active || (update.edit == Edit::Set && update.revision <= tracked.revision);
		}
		[[nodiscard]] bool _accept(const Update<State> &update)
		{
			auto found = _tracking.find(update.object);
			if (found != _tracking.end() && _obsolete(found->second, update))
			{
				return false;
			}
			if (found == _tracking.end() && _tracking.size() >= _maximumTracked)
			{
				throw spk::Exception("Tracking history full; reset session");
			}
			auto [entry, inserted] = _tracking.try_emplace(update.object);
			try
			{
				_apply(update);
			} catch (...)
			{
				if (inserted)
				{
					_tracking.erase(entry);
				}
				throw;
			}
			entry->second = {update.tracking, update.revision, update.edit == Edit::Set};
			return true;
		}
		[[nodiscard]] bool _receiveRejection(const spk::Message &message)
		{
			const auto reply = _protocol.decodeRequest(message, Protocol<State, Codec>::Kind::Rejected);
			auto found = _requests.find(reply.object);
			if (reply.session != _session || found == _requests.end() || found->second.id != reply.id || found->second.status != RequestStatus::Pending)
			{
				return false;
			}
			found->second.status = RequestStatus::Failed;
			return true;
		}

		[[nodiscard]] bool _sendRequest(const Request &request)
		{
			const auto message = _protocol.encode(request);
			auto [entry, inserted] = _requests.try_emplace(request.object, Acquisition{request.id});
			try
			{
				if (_sendMessage(message))
				{
					entry->second = {request.id};
					return true;
				}
			} catch (...)
			{
				if (inserted)
				{
					_requests.erase(entry);
				}
				throw;
			}
			if (inserted)
			{
				_requests.erase(entry);
			}
			return false;
		}

	protected:
		[[nodiscard]] virtual ReplicableTrait<State> *_findReplica(ObjectID id) = 0;
		// Creation must leave storage unchanged on failure. Removal must be idempotent,
		// and must succeed for a newly created replica whose initial application failed.
		[[nodiscard]] virtual ReplicableTrait<State> &_createReplica(ObjectID id) = 0;
		virtual void _removeReplica(ObjectID id) = 0;
		// Only needed by collections that explicitly request objects.
		[[nodiscard]] virtual bool _sendMessage(const spk::Message &)
		{
			throw spk::Exception("This replica collection has no request transport");
		}

	public:
		explicit ReplicaCollectionTrait(spk::Message::Type type, std::size_t maximumTracked = 65536, std::size_t maximumBytes = 2 * 1024 * 1024) :
			_protocol(type, maximumBytes),
			_maximumTracked(maximumTracked)
		{
			if (maximumTracked == 0)
			{
				throw spk::Exception("Invalid replica capacity");
			}
		}
		virtual ~ReplicaCollectionTrait() = default;
		ReplicaCollectionTrait(const ReplicaCollectionTrait &) = delete;
		ReplicaCollectionTrait &operator=(const ReplicaCollectionTrait &) = delete;
		void resetSession(SessionID session)
		{
			OperationGuard guard(_active);
			if (session.isNull())
			{
				throw spk::Exception("Null network session");
			}
			if (session != _session)
			{
				_clear();
				_session = session;
			}
		}
		void closeSession()
		{
			OperationGuard guard(_active);
			_clear();
			_session = {};
		}
		[[nodiscard]] bool receiveMessage(const spk::Message &message)
		{
			OperationGuard guard(_active);
			switch (_protocol.kind(message))
			{
			case Protocol<State, Codec>::Kind::Update:
				return _receiveUpdate(message);
			case Protocol<State, Codec>::Kind::Rejected:
				return _receiveRejection(message);
			default:
				throw spk::Exception("Unexpected request sent to replica collection");
			}
		}
		// No automatic retry policy. Call again to replace a request with a fresh ID.
		// false/exception means transport did not accept it; previous status is preserved.
		[[nodiscard]] bool requestObject(ObjectID id)
		{
			OperationGuard guard(_active);
			if (_session.isNull() || id.isNull() || (!_requests.contains(id) && _requests.size() >= _maximumTracked))
			{
				throw spk::Exception("Invalid request identity, session or capacity");
			}
			return _sendRequest({_session, id, _sequence.next()});
		}
		// Cancels local acquisition only. Server publication remains authoritative.
		void cancelRequest(ObjectID id)
		{
			OperationGuard guard(_active);
			_requests.erase(id);
		}
		[[nodiscard]] std::optional<RequestStatus> requestStatus(ObjectID id) const
		{
			auto found = _requests.find(id);
			return found == _requests.end() ? std::nullopt : std::optional{found->second.status};
		}
	};
}
