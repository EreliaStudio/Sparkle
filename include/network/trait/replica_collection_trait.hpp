#pragma once

#include "network/replication/protocol.hpp"
#include "network/replication/receiver.hpp"

namespace spk::Network
{
	// Owner-thread only. Derived collections own storage and object construction.
	template <typename State, typename Codec>
	class ReplicaCollectionTrait
	{
		Protocol<State, Codec> _protocol;
		Receiver<State> _receiver;
		bool _operating = false;

		void _applyUpdate(const Update<State> &update)
		{
			if (update.edit == Edit::Set)
			{
				_applyReplica(update.object, *update.state);
			}
			else
			{
				_removeReplica(update.object);
			}
		}

		void _clearReplicas()
		{
			for (auto id : _receiver.objects())
			{
				_removeReplica(id);
			}
		}

	protected:
		// Hooks must leave storage unchanged on failure. Removal is idempotent:
		// a termination can arrive without an earlier Set, or reset can retry.
		virtual void _applyReplica(ObjectID id, const State &state) = 0;
		virtual void _removeReplica(ObjectID id) = 0;
		virtual void _resetRequests(SessionID)
		{
		}
		virtual void _requireTransportIdle() const
		{
		}
		virtual bool _receiveReply(const Reply &, Clock::time_point)
		{
			throw spk::Exception("This replica collection does not request objects");
		}
		[[nodiscard]] const Protocol<State, Codec> &_protocolCodec() const noexcept
		{
			return _protocol;
		}
		[[nodiscard]] bool _hasReplica(ObjectID id) const
		{
			return _receiver.contains(id);
		}
		[[nodiscard]] SessionID _networkSession() const noexcept
		{
			return _receiver.session();
		}
		void _requireIdle() const
		{
			if (_operating)
			{
				throw spk::Exception("Replica collection mutation during application");
			}
			_requireTransportIdle();
		}

	public:
		explicit ReplicaCollectionTrait(spk::Message::Type type, std::size_t maximumTracked = 65536, std::size_t maximumBytes = 2 * 1024 * 1024) :
			_protocol(type, maximumBytes),
			_receiver(maximumTracked)
		{
		}
		virtual ~ReplicaCollectionTrait() = default;
		ReplicaCollectionTrait(const ReplicaCollectionTrait &) = delete;
		ReplicaCollectionTrait &operator=(const ReplicaCollectionTrait &) = delete;

		// Trusted handshake only. Repeating the current session preserves replicas.
		virtual void resetSession(SessionID session) final
		{
			_requireIdle();
			OperationGuard guard(_operating);
			if (session.isNull())
			{
				throw spk::Exception("Null network identity");
			}
			if (session == _receiver.session())
			{
				return;
			}
			_clearReplicas();
			_resetRequests(session);
			_receiver.reset(session);
		}

		virtual void closeSession() final
		{
			_requireIdle();
			OperationGuard guard(_operating);
			_clearReplicas();
			_resetRequests({});
			_receiver.close();
		}

		[[nodiscard]] virtual bool receiveMessage(const spk::Message &message, Clock::time_point now = Clock::now()) final
		{
			_requireIdle();
			OperationGuard guard(_operating);
			switch (_protocol.kind(message))
			{
			case Protocol<State, Codec>::Kind::Update:
				return _receiver.receive(_protocol.decodeUpdate(message), [this](const Update<State> &update) {
					_applyUpdate(update);
				});
			case Protocol<State, Codec>::Kind::Reply:
				return _receiveReply(_protocol.decodeReply(message), now);
			default:
				throw spk::Exception("Unexpected request sent to replica collection");
			}
		}
	};
}
