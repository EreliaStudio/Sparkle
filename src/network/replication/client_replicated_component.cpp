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

	void ClientReplicatedComponent::leaveInterest()
	{
		_onInterestLost();
		resetReceivedRevision();
	}

	void ClientReplicatedComponent::apply(const spk::ByteStream::Slice &reader, std::uint64_t revision)
	{
		if (_receivedRevision && revision <= *_receivedRevision)
		{
			return;
		}
		const spk::ByteStream state = _decodeByteStream(reader);
		if (!_validateByteStream(state))
		{
			throw spk::Exception("Invalid replicated state.");
		}
		_commitByteStream(state);
		_receivedRevision = revision;
	}
}
