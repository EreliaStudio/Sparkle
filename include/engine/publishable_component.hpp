#pragma once

#include "design_pattern/publishable.hpp"
#include "engine/component.hpp"

#include <string>

namespace spk
{
	class PublishableComponent : public Component, public Publishable::Part
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
		PublishableComponent(const std::string &name = "Unnamed publishable component", Entity *owner = nullptr);
		explicit PublishableComponent(Entity *owner);
	};
}
