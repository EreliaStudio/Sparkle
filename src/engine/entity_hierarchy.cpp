#include "engine/entity_hierarchy.hpp"

#include "exception.hpp"

#include <utility>

namespace spk
{
	EntityHierarchy::EntityHierarchy(MessageConfiguration configuration) :
		_messageConfiguration(configuration)
	{
		if (configuration.publication.type == configuration.initializationRequest.type ||
			configuration.publication.type == configuration.initializationResponse.type ||
			configuration.initializationRequest.type == configuration.initializationResponse.type)
		{
			throw spk::Exception("EntityHierarchy requires three distinct message IDs.");
		}
	}

	Entity &EntityHierarchy::root() noexcept
	{
		return _root;
	}

	const Entity &EntityHierarchy::root() const noexcept
	{
		return _root;
	}

	const EntityHierarchy::MessageConfiguration &EntityHierarchy::messageConfiguration() const noexcept
	{
		return _messageConfiguration;
	}
}
