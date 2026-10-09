#pragma once

#include "network/replication/replicated_component.hpp"
#include "network/message.hpp"

namespace spk::Network
{
	class ClientReplicatedComponent : public ReplicatedComponent
	{
	protected:
		// Implement using MementoTrait::transaction<State>() for rollback on malformed input.
		virtual void _readNetworkState(const spk::Message::Reader &reader) = 0;

	public:
		explicit ClientReplicatedComponent(spk::UUID identifier);
		~ClientReplicatedComponent() override = default;
		void apply(const spk::Message::Reader &reader);
	};
}
