#pragma once

#include "engine/system.hpp"
#include "network/replication/replicated_component.hpp"
#include "network/message.hpp"

namespace spk::Network
{
	class ReplicationSystem : public spk::System
	{
	protected:
		explicit ReplicationSystem(spk::Message::Type messageType);

		[[nodiscard]] ReplicatedComponent *find(const spk::UUID &identifier);
		[[nodiscard]] spk::Message::Type interestUpdateType() const noexcept;
		[[nodiscard]] spk::Message::Type interestRemovalType() const noexcept;
		[[nodiscard]] spk::Message::Type stateType() const noexcept;
		[[nodiscard]] spk::Message::Type componentRemovalType() const noexcept;

	private:
		spk::Message::Type _messageType;
	};
}
