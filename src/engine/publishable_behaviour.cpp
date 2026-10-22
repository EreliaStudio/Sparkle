#include "engine/publishable_behaviour.hpp"

#include "exception.hpp"

namespace spk
{
	PublishableBehaviour::PublishableBehaviour(const std::string &name, Entity *owner) :
		Behaviour(name, owner)
	{
		_activated = subscribeToActivation([this]() {
			invalidate();
		});
		_deactivated = subscribeToDeactivation([this]() {
			invalidate();
		});
	}

	PublishableBehaviour::PublishableBehaviour(Entity *owner) :
		PublishableBehaviour("Unnamed publishable behaviour", owner)
	{
	}

	void PublishableBehaviour::_encode(ByteStream::Writer &writer) const
	{
		writer << static_cast<std::uint8_t>(isActive());
		_encodeData(writer);
	}

	void PublishableBehaviour::_decode(const ByteStream::Reader &reader)
	{
		std::uint8_t active;
		reader >> active;
		if (active > 1)
		{
			throw spk::Exception("Invalid PublishableBehaviour activation state.");
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

	void PublishableBehaviour::_encodeData(ByteStream::Writer &) const
	{
	}

	void PublishableBehaviour::_decodeData(const ByteStream::Reader &)
	{
	}
}
