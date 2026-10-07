#include "network/trait/replica_collection_trait.hpp"

namespace spk::Network
{
	void ReplicaCollectionTrait::_requireClientIdle() const
	{
		_requireReplicaIdle();
	}
	bool &ReplicaCollectionTrait::_requestOperation()
	{
		return _replicaOperation();
	}
	SessionID ReplicaCollectionTrait::_requestSession() const
	{
		return _replicaSession();
	}
	bool ReplicaCollectionTrait::_sendObjectRequest(const Request &request)
	{
		return _sendMessage(_protocol.encode(request));
	}
	void ReplicaCollectionTrait::_onReplicasCleared()
	{
		_clearAcquisitions();
	}
	void ReplicaCollectionTrait::_onReplicaRemoved(ObjectID id)
	{
		_eraseAcquisition(id);
	}
	void ReplicaCollectionTrait::_onClientConnectionChanged()
	{
		closeSession();
		_handshakeToken = SessionID::generate();
		_helloSent = false;
	}
	void ReplicaCollectionTrait::_onClientTreatment()
	{
		if (!_helloSent && _clientConnected())
		{
			_helloSent = _sendToServer(_protocol.encodeHandshake(_handshakeToken));
		}
	}
	void ReplicaCollectionTrait::_onClientUnbinding()
	{
		closeSession();
		_handshakeToken = {};
		_helloSent = false;
	}
	void ReplicaCollectionTrait::_onClientMessage(const spk::Message &message)
	{
		if (_protocol.kind(message) == Protocol::Kind::Session)
		{
			const auto handshake = _protocol.decodeHandshake(message);
			if (handshake.token == _handshakeToken)
			{
				resetSession(handshake.session);
			}
		}
		else
		{
			(void)receiveMessage(message);
		}
	}
	bool ReplicaCollectionTrait::_receiveUpdate(const spk::Message &message)
	{
		const auto update = _protocol.decodeUpdate(message);
		if (update.session != _replicaSession() || update.session.isNull())
		{
			return false;
		}
		const bool accepted = _acceptUpdate(update);
		if (update.edit == Edit::Set && _tracksActive(update.object, update.tracking) &&
			_completeAcquisition(update.object, message.requestID()))
		{
			return true;
		}
		return accepted;
	}
	bool ReplicaCollectionTrait::_sendMessage(const spk::Message &message)
	{
		if (!isBound())
		{
			throw spk::Exception("This replica collection has no request transport");
		}
		return isSynchronized() && _sendToServer(message);
	}
	ReplicaCollectionTrait::ReplicaCollectionTrait(spk::Message::Type type, std::size_t maximumTracked, std::size_t maximumBytes) :
		ReplicaTrait(maximumTracked),
		ObjectRequesterTrait(maximumTracked),
		_protocol(type, maximumBytes)
	{
	}
	void ReplicaCollectionTrait::bind(spk::Client &client)
	{
		ClientBindingTrait::bind(client, _protocol.type());
	}
	bool ReplicaCollectionTrait::isSynchronized() const noexcept
	{
		return _connectionSynchronized() && !_replicaSession().isNull();
	}
	bool ReplicaCollectionTrait::receiveMessage(const spk::Message &message)
	{
		OperationGuard guard(_replicaOperation());
		switch (_protocol.kind(message))
		{
		case Protocol::Kind::Update:
			return _receiveUpdate(message);
		case Protocol::Kind::Rejected:
			return _rejectAcquisition(_protocol.decodeRequest(message, Protocol::Kind::Rejected));
		default:
			throw spk::Exception("Unexpected request sent to replica collection");
		}
	}
}
