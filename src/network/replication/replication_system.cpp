#include "network/replication/replication_system.hpp"

#include "engine/engine.hpp"
#include "engine/registry.hpp"
#include "exception.hpp"

#include <limits>

namespace spk::Network
{
	ReplicationSystem::ReplicationSystem(spk::Message::Type messageType) :
		spk::System("Replication"),
		_messageType(messageType)
	{
		if (_messageType > std::numeric_limits<spk::Message::Type>::max() - 3)
		{
			throw spk::Exception("Replication message types overflow.");
		}
	}

	ReplicatedComponent *ReplicationSystem::find(const spk::UUID &identifier)
	{
		if (engine() == nullptr)
		{
			return nullptr;
		}
		const auto &components = spk::Registry<spk::Component, spk::Engine *>::instance().elements(engine());
		for (spk::Component *item : components)
		{
			auto *component = dynamic_cast<ReplicatedComponent *>(item);
			if (component != nullptr && component->identifier() == identifier)
			{
				return component;
			}
		}
		return nullptr;
	}

	spk::Message::Type ReplicationSystem::interestUpdateType() const noexcept
	{
		return _messageType;
	}

	spk::Message::Type ReplicationSystem::interestRemovalType() const noexcept
	{
		return _messageType + 1;
	}

	spk::Message::Type ReplicationSystem::stateType() const noexcept
	{
		return _messageType + 2;
	}

	spk::Message::Type ReplicationSystem::componentRemovalType() const noexcept
	{
		return _messageType + 3;
	}
}
