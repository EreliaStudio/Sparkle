#pragma once

#include "engine/system.hpp"
#include "network/replication/replicated_component.hpp"
#include "network/message.hpp"

namespace spk::Network
{
	class ReplicationSystem : public spk::System
	{
	protected:
		explicit ReplicationSystem(spk::Message::Type requestType);

		[[nodiscard]] ReplicatedComponent *find(const spk::UUID &identifier);
		[[nodiscard]] spk::Message::Type requestType() const noexcept;
		[[nodiscard]] spk::Message::Type stateType() const noexcept;

	private:
		spk::Message::Type _requestType;
	};
}
