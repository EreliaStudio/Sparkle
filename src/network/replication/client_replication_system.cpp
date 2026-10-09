#include "network/replication/client_replication_system.hpp"

#include "engine/engine.hpp"
#include "engine/registry.hpp"
#include "exception.hpp"

#include <utility>

namespace spk::Network
{
	ClientReplicationSystem::ClientReplicationSystem(spk::Message::Type type) :
		ReplicationSystem(type)
	{
	}

	ClientReplicationSystem::~ClientReplicationSystem()
	{
		unbind();
	}

	void ClientReplicationSystem::bind(spk::Client &client)
	{
		if (_client == &client)
			return;
		unbind();
		auto stateContract = client.messageDispatcher().subscribeTo(stateType(),
			[this](const spk::Message &message) { onState(message); });
		auto disconnectionContract = client.subscribeToDisconnection([this] {
			resetReceivedRevisions();
		});
		_stateContract = std::move(stateContract);
		_disconnectionContract = std::move(disconnectionContract);
		_client = &client;
	}

	void ClientReplicationSystem::unbind()
	{
		_stateContract.resign();
		_disconnectionContract.resign();
		_client = nullptr;
		resetReceivedRevisions();
	}

	void ClientReplicationSystem::resetReceivedRevisions()
	{
		if (engine() == nullptr)
			return;
		const auto &components = spk::Registry<spk::Component, spk::Engine *>::instance().elements(engine());
		for (spk::Component *item : components)
		{
			if (auto *component = dynamic_cast<ClientReplicatedComponent *>(item))
				component->resetReceivedRevision();
		}
	}

	bool ClientReplicationSystem::isBound() const noexcept
	{
		return _client != nullptr;
	}

	void ClientReplicationSystem::onState(const spk::Message &message)
	{
		auto reader = message.reader();
		if (reader.size() < sizeof(spk::UUID::Storage) + sizeof(std::uint64_t))
			return;
		spk::UUID::Storage bytes{};
		std::uint64_t revision = 0;
		reader >> bytes >> revision;
		auto *component = dynamic_cast<ClientReplicatedComponent *>(find(spk::UUID(bytes)));
		if (component != nullptr)
			component->apply(reader, revision);
	}

	void ClientReplicationSystem::_updateState(spk::UpdateContext &)
	{
		if (_client != nullptr)
			_client->treatMessages();
	}

	void ClientReplicationSystem::request(const spk::UUID &identifier)
	{
		if (_client == nullptr)
			throw spk::Exception("Client replication system is not bound.");
		spk::Message::Writer writer(requestType());
		writer << identifier.bytes();
		_client->send(std::move(writer).build());
	}
}
