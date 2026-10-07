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
	void ClientBindingTrait::_onClientConnectionChanged()
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
		_connectionContract.resign();
		_disconnectionContract.resign();
		_connectionEdition.reset();
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
		return _clientConnected() && _observedConnection == _connectionEdition->load();
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
		const auto edition = _connectionEdition->load();
		if (_observedConnection != edition)
		{
			_onClientConnectionChanged();
			_observedConnection = edition;
		}
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
		auto edition = std::make_shared<std::atomic<std::uint64_t>>(1);
		auto connected = client.subscribeToConnection([edition] {
			++*edition;
		});
		auto disconnected = client.subscribeToDisconnection([edition] {
			++*edition;
		});
		auto messages = client.messageDispatcher().subscribeTo(type, [this](const auto &message) {
			_receiveClientMessage(message);
		});
		auto treatment = client.messageDispatcher().subscribeToTreatment([this] {
			_synchronizeClientBinding();
		});
		_client = &client;
		_type = type;
		_connectionEdition = std::move(edition);
		_observedConnection = 0;
		_connectionContract = std::move(connected);
		_disconnectionContract = std::move(disconnected);
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
