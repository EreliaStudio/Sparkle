#include "network/replication/server_replication_system.hpp"

#include "engine/engine.hpp"
#include "engine/registry.hpp"

#include <utility>

namespace spk::Network
{
	ServerReplicationSystem::ServerReplicationSystem(spk::Message::Type type) :
		ReplicationSystem(type)
	{
	}

	ServerReplicationSystem::~ServerReplicationSystem()
	{
		unbind();
	}

	void ServerReplicationSystem::bind(spk::Server &server)
	{
		if (_server == &server)
			return;
		unbind();
		auto requestContract = server.messageDispatcher().subscribeTo(requestType(),
			[this](const spk::ReceivedMessage &incoming) { onRequest(incoming); });
		auto connectionContract = server.subscribeToConnection([this](spk::ConnectionID peer) {
			const std::scoped_lock lock(_peerMutex);
			_peers.insert(peer);
		});
		auto disconnectionContract = server.subscribeToDisconnection([this](spk::ConnectionID peer) {
			const std::scoped_lock lock(_peerMutex);
			_peers.erase(peer);
			_sent.erase(peer);
		});
		_requestContract = std::move(requestContract);
		_connectionContract = std::move(connectionContract);
		_disconnectionContract = std::move(disconnectionContract);
		_server = &server;
	}

	void ServerReplicationSystem::unbind()
	{
		_requestContract.resign();
		_connectionContract.resign();
		_disconnectionContract.resign();
		_server = nullptr;
		const std::scoped_lock lock(_peerMutex);
		_peers.clear();
		_sent.clear();
	}

	bool ServerReplicationSystem::isBound() const noexcept
	{
		return _server != nullptr;
	}

	void ServerReplicationSystem::setRequestAuthorizer(Authorizer authorizer)
	{
		_authorizer = std::move(authorizer);
	}

	spk::Message ServerReplicationSystem::stateMessage(const ServerReplicatedComponent &component) const
	{
		spk::Message::Writer writer(stateType());
		writer << component.identifier().bytes() << component.version();
		component.capture(writer);
		return std::move(writer).build();
	}

	void ServerReplicationSystem::sendUpdates()
	{
		if (_server == nullptr || engine() == nullptr)
			return;
		const std::scoped_lock lock(_peerMutex);
		for (spk::ConnectionID peer : _peers)
		{
			const auto &components = spk::Registry<spk::Component, spk::Engine *>::instance().elements(engine());
			for (spk::Component *item : components)
			{
				auto *component = dynamic_cast<ServerReplicatedComponent *>(item);
				if (component == nullptr || !_authorizer || !_authorizer(peer, *component))
					continue;
				auto &versions = _sent[peer];
				auto previous = versions.find(component->identifier());
				if (previous != versions.end() && previous->second == component->version())
					continue;
				_server->sendTo(peer, stateMessage(*component));
				versions[component->identifier()] = component->version();
			}
		}
	}

	void ServerReplicationSystem::onRequest(const spk::ReceivedMessage &incoming)
	{
		auto reader = incoming.message.reader();
		if (reader.size() != sizeof(spk::UUID::Storage))
			return;
		spk::UUID::Storage bytes{};
		reader >> bytes;
		auto *component = dynamic_cast<ServerReplicatedComponent *>(find(spk::UUID(bytes)));
		if (component == nullptr || !_authorizer || !_authorizer(incoming.emitter, *component))
			return;
		_server->sendTo(incoming.emitter, stateMessage(*component));
		const std::scoped_lock lock(_peerMutex);
		_sent[incoming.emitter][component->identifier()] = component->version();
	}

	void ServerReplicationSystem::_updateState(spk::UpdateContext &)
	{
		if (_server == nullptr)
			return;
		_server->treatMessages();
		sendUpdates();
	}
}
