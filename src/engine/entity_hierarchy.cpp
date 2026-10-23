#include "engine/entity_hierarchy.hpp"

#include "exception.hpp"

#include <utility>

namespace spk
{
	EntityHierarchy::EntityHierarchy(MessageTypes types, Priorities priorities) :
		_messageTypes(types),
		_priorities(priorities)
	{
		if (types.publication == types.initializationRequest ||
			types.publication == types.initializationResponse ||
			types.initializationRequest == types.initializationResponse)
		{
			throw spk::Exception("EntityHierarchy requires three distinct message IDs.");
		}
	}

	EntityHierarchy::EntityHierarchy(MessageTypes types) :
		EntityHierarchy(types, Priorities{})
	{
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

	const EntityHierarchy::MessageTypes &EntityHierarchy::messageTypes() const noexcept
	{
		return _messageTypes;
	}

	const EntityHierarchy::Priorities &EntityHierarchy::priorities() const noexcept
	{
		return _priorities;
	}
}
