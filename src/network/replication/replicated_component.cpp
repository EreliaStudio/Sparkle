#include "network/replication/replicated_component.hpp"

#include "exception.hpp"

namespace spk::Network
{
	ReplicatedComponent::ReplicatedComponent(spk::UUID identifier) :
		spk::Component("Replicated component"), _identifier(identifier)
	{
		if (_identifier.isNull())
			throw spk::Exception("Replicated component identifier must not be null.");
	}

	const spk::UUID &ReplicatedComponent::identifier() const noexcept
	{
		return _identifier;
	}
}
