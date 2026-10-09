#include "network/replication/server_replicated_component.hpp"

namespace spk::Network
{
	ServerReplicatedComponent::ServerReplicatedComponent(spk::UUID identifier) :
		ReplicatedComponent(identifier)
	{
	}

	void ServerReplicatedComponent::capture(spk::Message::Writer &writer) const
	{
		_writeNetworkState(writer);
	}
}
