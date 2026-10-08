#include "network/replication/replication_system.hpp"

#include "engine/engine.hpp"
#include "engine/registry.hpp"
#include "exception.hpp"

#include <limits>
#include <utility>
#include <vector>

namespace spk::Network
{
	namespace
	{
		void checkMessageType(spk::Message::Type type)
		{
			if (type == std::numeric_limits<spk::Message::Type>::max())
				throw spk::Exception("Replication message type has no state successor.");
		}
	}

	ReplicationSystem::ReplicationSystem(spk::Client &client, spk::Message::Type type) :
		spk::System("Replication"), _client(&client), _requestType(type), _stateType(type + 1)
	{
		checkMessageType(type);
		_clientStateContract = client.messageDispatcher().subscribeTo(_stateType,
			[this](const spk::Message &message) { onState(message); });
	}

	ReplicationSystem::ReplicationSystem(spk::Server &server, spk::Message::Type type) :
		spk::System("Replication"), _server(&server), _requestType(type), _stateType(type + 1)
	{
		checkMessageType(type);
		_serverRequestContract = server.messageDispatcher().subscribeTo(_requestType,
			[this](const spk::ReceivedMessage &incoming) { onRequest(incoming); });
		_connectionContract = server.subscribeToConnection([this](spk::ConnectionID peer) {
			const std::scoped_lock lock(_peerMutex);
			_peers.insert(peer);
		});
		_disconnectionContract = server.subscribeToDisconnection([this](spk::ConnectionID peer) {
			const std::scoped_lock lock(_peerMutex);
			_peers.erase(peer);
			_sent.erase(peer);
		});
	}

	ReplicationSystem::~ReplicationSystem()
	{
		_clientStateContract.resign();
		_serverRequestContract.resign();
		_connectionContract.resign();
		_disconnectionContract.resign();
	}

	void ReplicationSystem::setRequestAuthorizer(Authorizer authorizer)
	{
		_authorizer = std::move(authorizer);
	}

	ReplicatedComponent *ReplicationSystem::find(const spk::UUID &id)
	{
		if (engine() == nullptr)
			return nullptr;
		for (spk::Component *item : spk::Registry<spk::Component, spk::Engine *>::instance().elements(engine()))
		{
			auto *component = dynamic_cast<ReplicatedComponent *>(item);
			if (component != nullptr && component->identifier() == id)
				return component;
		}
		return nullptr;
	}

	spk::Message ReplicationSystem::stateMessage(const ReplicatedComponent &component) const
	{
		spk::Message::Writer writer(_stateType);
		writer << component.identifier().bytes() << component.version();
		component.capture(writer);
		return std::move(writer).build();
	}

	void ReplicationSystem::sendUpdates()
	{
		const std::scoped_lock lock(_peerMutex);
		if (engine() == nullptr)
			return;
		for (spk::ConnectionID peer : _peers)
		{
			for (spk::Component *item : spk::Registry<spk::Component, spk::Engine *>::instance().elements(engine()))
			{
				auto *component = dynamic_cast<ReplicatedComponent *>(item);
				if (component == nullptr || component->mode() != ReplicatedComponent::Mode::Authoritative)
					continue;
				if (!_authorizer || !_authorizer(peer, *component))
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

	void ReplicationSystem::onRequest(const spk::ReceivedMessage &incoming)
	{
		auto reader = incoming.message.reader();
		if (reader.size() != sizeof(spk::UUID::Storage))
			return;
		spk::UUID::Storage bytes{};
		reader >> bytes;
		ReplicatedComponent *component = find(spk::UUID(bytes));
		if (component == nullptr || component->mode() != ReplicatedComponent::Mode::Authoritative)
			return;
		if (!_authorizer || !_authorizer(incoming.emitter, *component))
			return;
		_server->sendTo(incoming.emitter, stateMessage(*component));
		const std::scoped_lock lock(_peerMutex);
		_sent[incoming.emitter][component->identifier()] = component->version();
	}

	void ReplicationSystem::onState(const spk::Message &message)
	{
		auto reader = message.reader();
		if (reader.size() < sizeof(spk::UUID::Storage) + sizeof(std::uint64_t))
			return;
		spk::UUID::Storage bytes{};
		std::uint64_t revision = 0;
		reader >> bytes >> revision;
		const spk::UUID identifier(bytes);
		ReplicatedComponent *component = find(identifier);
		if (component == nullptr || component->mode() != ReplicatedComponent::Mode::Replica)
			return;
		auto previous = _received.find(identifier);
		if (previous != _received.end() && revision <= previous->second)
			return;
		component->apply(reader);
		if (reader.readOffset() != reader.size())
			throw spk::Exception("Replication payload contains trailing bytes.");
		_received[identifier] = revision;
	}

	void ReplicationSystem::_updateState(spk::UpdateContext &)
	{
		if (_client != nullptr)
			_client->treatMessages();
		if (_server != nullptr)
		{
			_server->treatMessages();
			sendUpdates();
		}
	}

	void ReplicationSystem::request(const spk::UUID &identifier)
	{
		if (_client == nullptr)
			throw spk::Exception("Only replicas can request component state.");
		spk::Message::Writer writer(_requestType);
		writer << identifier.bytes();
		_client->send(std::move(writer).build());
	}
}
