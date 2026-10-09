#pragma once

#include "network/replication/replicated_component.hpp"
#include "network/message.hpp"

#include <cstdint>
#include <optional>

namespace spk::Network
{
	class ClientReplicatedComponent : public ReplicatedComponent
	{
	private:
		std::optional<std::uint64_t> _receivedRevision;

	protected:
		// Implement using MementoTrait::transaction<State>() for rollback on malformed input.
		virtual void _readNetworkState(const spk::Message::Reader &reader) = 0;
		virtual void _onInterestLost() {}

	public:
		explicit ClientReplicatedComponent(spk::UUID identifier);
		~ClientReplicatedComponent() override = default;

		[[nodiscard]] std::optional<std::uint64_t> receivedRevision() const noexcept;
		void resetReceivedRevision() noexcept;
		void leaveInterest();
		void apply(const spk::Message::Reader &reader);
		void apply(const spk::Message::Reader &reader, std::uint64_t revision);
	};
}
