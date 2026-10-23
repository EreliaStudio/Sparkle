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

	void EntityHierarchy::_registerSourceType(std::type_index type, EntityTypeID id)
	{
		if (_sourceTypes.contains(type))
		{
			throw spk::Exception("EntityHierarchy concrete Source type already registered.");
		}
		for (const auto &[existingType, existingID] : _sourceTypes)
		{
			if (existingID == id)
			{
				throw spk::Exception("EntityHierarchy EntityTypeID is already registered.");
			}
		}
		_sourceTypes.emplace(type, id);
	}

	void EntityHierarchy::_registerReplicantType(EntityTypeID id, ReplicantFactory factory)
	{
		if (!factory || _replicantFactories.contains(id))
		{
			throw spk::Exception("Invalid or duplicate EntityHierarchy Replicant factory.");
		}
		_replicantFactories.emplace(id, std::move(factory));
	}

	void EntityHierarchy::_clearTypes() noexcept
	{
		_sourceTypes.clear();
		_replicantFactories.clear();
	}

	const EntityHierarchy::MessageConfiguration &EntityHierarchy::messageConfiguration() const noexcept
	{
		return _messageConfiguration;
	}
}
