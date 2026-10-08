#include "network/replication/replicated_component.hpp"

#include "exception.hpp"

namespace spk::Network
{
	ReplicatedComponent::ReplicatedComponent(spk::UUID identifier, Mode mode) :
		spk::Component("Replicated component"), _identifier(identifier), _mode(mode)
	{
		if (_identifier.isNull())
			throw spk::Exception("Replicated component identifier must not be null.");
	}

	const spk::UUID &ReplicatedComponent::identifier() const noexcept
	{
		return _identifier;
	}

	ReplicatedComponent::Mode ReplicatedComponent::mode() const noexcept
	{
		return _mode;
	}

	void ReplicatedComponent::capture(spk::Message::Writer &writer) const
	{
		if (_mode != Mode::Authoritative)
			throw spk::Exception("Cannot publish a replica component.");
		_writeNetworkState(writer);
	}

	void ReplicatedComponent::apply(const spk::Message::Reader &reader)
	{
		if (_mode != Mode::Replica)
			throw spk::Exception("Cannot overwrite authoritative component.");
		_readNetworkState(reader);
	}
}
