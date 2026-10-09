#include "network/replication/client_replication_system.hpp"

#include "exception.hpp"

#include <utility>

namespace spk::Network
{
	ClientReplicationSystem::ClientReplicationSystem(spk::Client &client, spk::Message::Type type) :
		ReplicationSystem(type), _client(client)
	{
		_stateContract = client.messageDispatcher().subscribeTo(stateType(),
			[this](const spk::Message &message) { onState(message); });
	}

	ClientReplicationSystem::~ClientReplicationSystem()
	{
		_stateContract.resign();
	}

	void ClientReplicationSystem::onState(const spk::Message &message)
	{
		auto reader = message.reader();
		if (reader.size() < sizeof(spk::UUID::Storage) + sizeof(std::uint64_t))
			return;
		spk::UUID::Storage bytes{};
		std::uint64_t revision = 0;
		reader >> bytes >> revision;
		const spk::UUID identifier(bytes);
		auto *component = dynamic_cast<ClientReplicatedComponent *>(find(identifier));
		if (component == nullptr)
			return;
		auto previous = _received.find(identifier);
		if (previous != _received.end() && revision <= previous->second)
			return;
		component->apply(reader);
		if (reader.readOffset() != reader.size())
			throw spk::Exception("Replication payload contains trailing bytes.");
		_received[identifier] = revision;
	}

	void ClientReplicationSystem::_updateState(spk::UpdateContext &)
	{
		_client.treatMessages();
	}

	void ClientReplicationSystem::request(const spk::UUID &identifier)
	{
		spk::Message::Writer writer(requestType());
		writer << identifier.bytes();
		_client.send(std::move(writer).build());
	}
}
