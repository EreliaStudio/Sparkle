#include "network/replication/replication_system.hpp"

#include "engine/engine.hpp"
#include "engine/registry.hpp"
#include "exception.hpp"

#include <limits>

namespace spk::Network
{
	ReplicationSystem::ReplicationSystem(spk::Message::Type requestType) :
		spk::System("Replication"), _requestType(requestType)
	{
		if (_requestType == std::numeric_limits<spk::Message::Type>::max())
			throw spk::Exception("Replication message type has no state successor.");
	}

	ReplicatedComponent *ReplicationSystem::find(const spk::UUID &identifier)
	{
		if (engine() == nullptr)
			return nullptr;
		const auto &components = spk::Registry<spk::Component, spk::Engine *>::instance().elements(engine());
		for (spk::Component *item : components)
		{
			auto *component = dynamic_cast<ReplicatedComponent *>(item);
			if (component != nullptr && component->identifier() == identifier)
				return component;
		}
		return nullptr;
	}

	spk::Message::Type ReplicationSystem::requestType() const noexcept
	{
		return _requestType;
	}

	spk::Message::Type ReplicationSystem::stateType() const noexcept
	{
		return _requestType + 1;
	}
}
