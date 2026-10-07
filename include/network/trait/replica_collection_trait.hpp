#pragma once
#include "client_binding_trait.hpp"
#include "network/replication/protocol.hpp"
#include "object_requester_trait.hpp"
#include "replica_trait.hpp"
namespace spk::Network
{
	// Complete client replication channel. Application storage remains in replica hooks.
	template <typename State, typename Codec>
	class ReplicaCollectionTrait : public ReplicaTrait<State>, public ObjectRequesterTrait, protected ClientBindingTrait
	{
		Protocol<State, Codec> _protocol;
		SessionID _handshakeToken;
		bool _helloSent = false;
		void _requireClientIdle() const override
		{
			this->_requireReplicaIdle();
		}
		bool &_requestOperation() override
		{
			return this->_replicaOperation();
		}
		SessionID _requestSession() const override
		{
			return this->_replicaSession();
		}
		bool _sendObjectRequest(const Request &request) override
		{
			return _sendMessage(_protocol.encode(request));
		}
		void _onReplicasCleared() override
		{
			_clearAcquisitions();
		}
		void _onReplicaRemoved(ObjectID id) override
		{
			_eraseAcquisition(id);
		}
		void _onClientConnectionChanged() override
		{
			this->closeSession();
			_handshakeToken = SessionID::generate();
			_helloSent = false;
		}
		void _onClientTreatment() override
		{
			if (!_helloSent && _clientConnected())
			{
				_helloSent = _sendToServer(_protocol.encodeHandshake(_handshakeToken));
			}
		}
		void _onClientUnbinding() override
		{
			this->closeSession();
			_handshakeToken = {};
			_helloSent = false;
		}
		void _onClientMessage(const spk::Message &message) override
		{
			if (_protocol.kind(message) == Protocol<State, Codec>::Kind::Session)
			{
				const auto handshake = _protocol.decodeHandshake(message);
				if (handshake.token == _handshakeToken)
				{
					this->resetSession(handshake.session);
				}
			}
			else
			{
				(void)receiveMessage(message);
			}
		}
		bool _receiveUpdate(const spk::Message &message)
		{
			const auto update = _protocol.decodeUpdate(message);
			if (update.session != this->_replicaSession() || update.session.isNull())
			{
				return false;
			}
			const bool accepted = this->_acceptUpdate(update);
			if (update.edit == Edit::Set && this->_tracksActive(update.object, update.tracking) &&
				_completeAcquisition(update.object, message.requestID()))
			{
				return true;
			}
			return accepted;
		}

	protected:
		[[nodiscard]] virtual bool _sendMessage(const spk::Message &message)
		{
			if (!isBound())
			{
				throw spk::Exception("This replica collection has no request transport");
			}
			return isSynchronized() && _sendToServer(message);
		}

	public:
		explicit ReplicaCollectionTrait(spk::Message::Type type, std::size_t maximumTracked = 65536, std::size_t maximumBytes = 2 * 1024 * 1024) :
			ReplicaTrait<State>(maximumTracked),
			ObjectRequesterTrait(maximumTracked),
			_protocol(type, maximumBytes)
		{
		}
		~ReplicaCollectionTrait() override = default;
		void bind(spk::Client &client)
		{
			ClientBindingTrait::bind(client, _protocol.type());
		}
		using ClientBindingTrait::isBound;
		using ClientBindingTrait::unbind;
		[[nodiscard]] bool isSynchronized() const noexcept
		{
			return _connectionSynchronized() && !this->_replicaSession().isNull();
		}
		[[nodiscard]] bool receiveMessage(const spk::Message &message)
		{
			OperationGuard guard(this->_replicaOperation());
			switch (_protocol.kind(message))
			{
			case Protocol<State, Codec>::Kind::Update:
				return _receiveUpdate(message);
			case Protocol<State, Codec>::Kind::Rejected:
				return _rejectAcquisition(_protocol.decodeRequest(message, Protocol<State, Codec>::Kind::Rejected));
			default:
				throw spk::Exception("Unexpected request sent to replica collection");
			}
		}
	};
}
