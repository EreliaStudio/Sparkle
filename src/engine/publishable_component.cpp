#include "engine/publishable_component.hpp"

#include "exception.hpp"

namespace spk
{
	PublishableComponent::PublishableComponent(const std::string &name, Entity *owner) :
		Component(name, owner)
	{
		_activated = subscribeToActivation([this]() {
			invalidate();
		});
		_deactivated = subscribeToDeactivation([this]() {
			invalidate();
		});
	}

	PublishableComponent::PublishableComponent(Entity *owner) :
		PublishableComponent("Unnamed publishable component", owner)
	{
	}

	void PublishableComponent::_encode(ByteStream::Writer &writer) const
	{
		writer << static_cast<std::uint8_t>(isActive());
		_encodeData(writer);
	}

	void PublishableComponent::_decode(const ByteStream::Reader &reader)
	{
		std::uint8_t active;
		reader >> active;
		if (active > 1)
		{
			throw spk::Exception("Invalid PublishableComponent activation state.");
		}
		_decodeData(reader);
		if (active != 0)
		{
			activate();
		}
		else
		{
			deactivate();
		}
	}

}
