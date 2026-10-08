#include "network/trait/client_binding_trait.hpp"
#include "exception.hpp"
#include "network/replication/operation_guard.hpp"
namespace spk::Network
{
	ClientBindingTrait::~ClientBindingTrait()
	{
		_releaseClientBinding();
	}
	void ClientBindingTrait::_requireClientIdle() const
	{
	}
	void ClientBindingTrait::_onClientTreatment()
	{
	}
	void ClientBindingTrait::_onClientUnbinding()
	{
	}
	void ClientBindingTrait::_requireBindingIdle() const
	{
		_requireClientIdle();
		if (_handling)
		{
			throw spk::Exception("Client binding mutation during treatment");
		}
	}
	void ClientBindingTrait::_releaseClientBinding() noexcept
	{
		_messageContract.resign();
		_treatmentContract.resign();
		_releaseClientObservation();
		_client = nullptr;
	}
	bool ClientBindingTrait::isBound() const noexcept
	{
		return _messageContract.isValid();
	}
	bool ClientBindingTrait::_clientConnected() const noexcept
	{
		return isBound() && _client->isConnected();
	}
	bool ClientBindingTrait::_connectionSynchronized() const noexcept
	{
		return _clientConnected() && _clientConnectionObserved();
	}
	bool ClientBindingTrait::_sendToServer(const spk::Message &message)
	{
		if (!_clientConnected())
		{
			return false;
		}
		_client->send(message);
		return true;
	}
	void ClientBindingTrait::_synchronizeClientBinding()
	{
		_requireBindingIdle();
		OperationGuard guard(_handling);
		_synchronizeClientConnection();
		_onClientTreatment();
	}
	void ClientBindingTrait::_receiveClientMessage(const spk::Message &message)
	{
		_synchronizeClientBinding();
		if (_clientConnected())
		{
			OperationGuard guard(_handling);
			_onClientMessage(message);
		}
	}
	void ClientBindingTrait::_subscribe(spk::Client &client, spk::Message::Type type)
	{
		auto messages = client.messageDispatcher().subscribeTo(type, [this](const auto &message) {
			_receiveClientMessage(message);
		});
		auto treatment = client.messageDispatcher().subscribeToTreatment([this] {
			_synchronizeClientBinding();
		});
		_observeClientConnection(client);
		_client = &client;
		_type = type;
		_messageContract = std::move(messages);
		_treatmentContract = std::move(treatment);
	}
	void ClientBindingTrait::bind(spk::Client &client, spk::Message::Type type)
	{
		_requireBindingIdle();
		if (_client == &client && _type == type && isBound())
		{
			return;
		}
		unbind();
		_subscribe(client, type);
	}
	void ClientBindingTrait::unbind()
	{
		_requireBindingIdle();
		OperationGuard guard(_handling);
		_onClientUnbinding();
		_releaseClientBinding();
	}
}
