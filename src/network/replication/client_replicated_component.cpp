#include "network/replication/client_replicated_component.hpp"

#include "exception.hpp"

namespace spk::Network
{
	ClientReplicatedComponent::ClientReplicatedComponent(spk::UUID identifier) :
		ReplicatedComponent(identifier)
	{
	}

	std::optional<std::uint64_t> ClientReplicatedComponent::receivedRevision() const noexcept
	{
		return _receivedRevision;
	}

	void ClientReplicatedComponent::resetReceivedRevision() noexcept
	{
		_receivedRevision.reset();
	}

	void ClientReplicatedComponent::apply(const spk::Message::Reader &reader)
	{
		_readNetworkState(reader);
	}

	void ClientReplicatedComponent::apply(const spk::Message::Reader &reader, std::uint64_t revision)
	{
		if (_receivedRevision && revision <= *_receivedRevision)
			return;
		_readNetworkState(reader);
		if (reader.readOffset() != reader.size())
			throw spk::Exception("Replication payload contains trailing bytes.");
		_receivedRevision = revision;
	}
}
