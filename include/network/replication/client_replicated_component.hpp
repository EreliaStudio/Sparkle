#pragma once

#include "container/byte_stream.hpp"
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
		[[nodiscard]] virtual spk::ByteStream _decodeByteStream(const spk::ByteStream::Slice &reader) const = 0;
		[[nodiscard]] virtual bool _validateByteStream(const spk::ByteStream &state) const = 0;
		virtual void _commitByteStream(const spk::ByteStream &state) = 0;
		virtual void _onInterestLost()
		{
		}

	public:
		explicit ClientReplicatedComponent(spk::UUID identifier);
		~ClientReplicatedComponent() override = default;

		[[nodiscard]] std::optional<std::uint64_t> receivedRevision() const noexcept;
		void resetReceivedRevision() noexcept;
		void leaveInterest();
		void apply(const spk::ByteStream::Slice &reader, std::uint64_t revision);
	};
}
