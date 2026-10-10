#pragma once

#include "design_pattern/trait/versioned_trait.hpp"
#include "network/message.hpp"
#include "network/replication/replicated_component.hpp"

namespace spk::Network
{
	class ServerReplicatedComponent : public ReplicatedComponent, public spk::VersionedTrait
	{
	protected:
		virtual void _writeNetworkState(spk::Message::Writer &writer) const = 0;

	public:
		explicit ServerReplicatedComponent(spk::UUID identifier);
		~ServerReplicatedComponent() override = default;
		void capture(spk::Message::Writer &writer) const;
	};
}
