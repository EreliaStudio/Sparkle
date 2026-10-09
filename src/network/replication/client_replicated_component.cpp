#include "network/replication/client_replicated_component.hpp"

namespace spk::Network
{
	ClientReplicatedComponent::ClientReplicatedComponent(spk::UUID identifier) :
		ReplicatedComponent(identifier)
	{
	}

	void ClientReplicatedComponent::apply(const spk::Message::Reader &reader)
	{
		_readNetworkState(reader);
	}
}
