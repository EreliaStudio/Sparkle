#include "network/trait/server_binding_trait.hpp"
#include "exception.hpp"
#include "network/replication/operation_guard.hpp"
namespace spk::Network
{
	ServerBindingTrait::~ServerBindingTrait()
	{
		_releaseServerBinding();
	}
	void ServerBindingTrait::_requireServerIdle() const
	{
	}
	void ServerBindingTrait::_onServerUnbinding()
	{
	}
	void ServerBindingTrait::_requireBindingIdle() const
	{
		_requireServerIdle();
		if (_handling)
		{
			throw spk::Exception("Server binding mutation during treatment");
		}
	}
	void ServerBindingTrait::_releaseServerBinding() noexcept
	{
		_messageContract.resign();
		_treatmentContract.resign();
		_releaseServerObservation();
		_server = nullptr;
	}
	bool ServerBindingTrait::isBound() const noexcept
	{
		return _messageContract.isValid();
	}
	bool ServerBindingTrait::_sendTo(spk::ConnectionID connection, const spk::Message &message)
	{
		if (!isBound() || !_connectionLive(connection))
		{
			return false;
		}
		_server->sendTo(connection, message);
		return true;
	}
	void ServerBindingTrait::_receiveServerMessage(const spk::ReceivedMessage &message)
	{
		_synchronizeServerBinding();
		if (_connectionLive(message.emitter))
		{
			OperationGuard guard(_handling);
			_onServerMessage(message);
		}
	}
	void ServerBindingTrait::_synchronizeServerBinding()
	{
		_requireBindingIdle();
		OperationGuard guard(_handling);
		_synchronizeServerConnections();
	}
	void ServerBindingTrait::_subscribe(spk::Server &server, spk::Message::Type type)
	{
		auto messages = server.messageDispatcher().subscribeTo(type, [this](const auto &message) {
			_receiveServerMessage(message);
		});
		auto treatment = server.messageDispatcher().subscribeToTreatment([this] {
			_synchronizeServerBinding();
		});
		_observeServerConnections(server);
		_server = &server;
		_type = type;
		_messageContract = std::move(messages);
		_treatmentContract = std::move(treatment);
	}
	void ServerBindingTrait::bind(spk::Server &server, spk::Message::Type type)
	{
		_requireBindingIdle();
		if (_server == &server && _type == type && isBound())
		{
			return;
		}
		if (server.isRunning())
		{
			throw spk::Exception("Bind publication before starting the server");
		}
		unbind();
		_subscribe(server, type);
	}
	void ServerBindingTrait::unbind()
	{
		_requireBindingIdle();
		OperationGuard guard(_handling);
		_onServerUnbinding();
		_releaseServerBinding();
	}
}
