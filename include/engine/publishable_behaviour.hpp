#pragma once

#include "design_pattern/publishable.hpp"
#include "engine/behaviour.hpp"

#include <string>

namespace spk
{
	class PublishableBehaviour : public Behaviour, public Publishable::Part
	{
	private:
		ActivationContract _activated;
		DeactivationContract _deactivated;

		void _encode(ByteStream::Writer &writer) const final;
		void _decode(const ByteStream::Reader &reader) final;

	protected:
		virtual void _encodeData(ByteStream::Writer &) const
		{
		}
		virtual void _decodeData(const ByteStream::Reader &)
		{
		}

	public:
		PublishableBehaviour(const std::string &name = "Unnamed publishable behaviour", Entity *owner = nullptr);
		explicit PublishableBehaviour(Entity *owner);
	};
}
