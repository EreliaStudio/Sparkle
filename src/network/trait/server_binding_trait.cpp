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
	void ServerBindingTrait::_onServerConnectionOpened(spk::ConnectionID)
	{
	}
	void ServerBindingTrait::_onServerConnectionClosed(spk::ConnectionID)
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
		_connectionContract.resign();
		_disconnectionContract.resign();
		_signals.reset();
		_observed.clear();
		_server = nullptr;
	}
	bool ServerBindingTrait::isBound() const noexcept
	{
		return _messageContract.isValid();
	}
	bool ServerBindingTrait::_connectionLive(spk::ConnectionID connection) const
	{
		if (!isBound())
		{
			return false;
		}
		const std::scoped_lock lock(_signals->mutex);
		return _signals->live.contains(connection);
	}
	bool ServerBindingTrait::_sendTo(spk::ConnectionID connection, const spk::Message &message)
	{
		if (!_connectionLive(connection))
		{
			return false;
		}
		_server->sendTo(connection, message);
		return true;
	}
	void ServerBindingTrait::_synchronizeServerBinding()
	{
		_requireBindingIdle();
		OperationGuard guard(_handling);
		std::set<spk::ConnectionID> live;
		{
			const std::scoped_lock lock(_signals->mutex);
			live = _signals->live;
		}
		std::erase_if(_observed, [&](spk::ConnectionID connection) {
			if (live.contains(connection))
			{
				return false;
			}
			_onServerConnectionClosed(connection);
			return true;
		});
		for (auto connection : live)
		{
			if (!_observed.contains(connection))
			{
				_onServerConnectionOpened(connection);
				_observed.insert(connection);
			}
		}
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
	void ServerBindingTrait::_subscribe(spk::Server &server, spk::Message::Type type)
	{
		auto signals = std::make_shared<Signals>();
		auto connected = server.subscribeToConnection([signals](spk::ConnectionID id) {
			const std::scoped_lock lock(signals->mutex);
			signals->live.insert(id);
		});
		auto disconnected = server.subscribeToDisconnection([signals](spk::ConnectionID id) {
			const std::scoped_lock lock(signals->mutex);
			signals->live.erase(id);
		});
		auto messages = server.messageDispatcher().subscribeTo(type, [this](const auto &message) {
			_receiveServerMessage(message);
		});
		auto treatment = server.messageDispatcher().subscribeToTreatment([this] {
			_synchronizeServerBinding();
		});
		_server = &server;
		_signals = std::move(signals);
		_type = type;
		_connectionContract = std::move(connected);
		_disconnectionContract = std::move(disconnected);
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
