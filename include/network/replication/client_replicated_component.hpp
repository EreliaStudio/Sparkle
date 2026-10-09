#pragma once

#include "network/message.hpp"
#include "network/replication/replicated_component.hpp"

#include <cstdint>
#include <optional>

namespace spk::Network
{
	class ClientReplicatedComponent : public ReplicatedComponent
	{
	private:
		std::optional<std::uint64_t> _receivedRevision;

	protected:
		// Decode into a private pending state; never mutate the live state here.
		virtual void _decodeNetworkState(const spk::Message::Reader &reader) = 0;
		// Commit validated pending state without throwing.
		virtual void _commitNetworkState() noexcept = 0;
		virtual void _onInterestLost()
		{
		}

	public:
		explicit ClientReplicatedComponent(spk::UUID identifier);
		~ClientReplicatedComponent() override = default;

		[[nodiscard]] std::optional<std::uint64_t> receivedRevision() const noexcept;
		void resetReceivedRevision() noexcept;
		void leaveInterest();
		void apply(const spk::Message::Reader &reader, std::uint64_t revision);
	};
}
