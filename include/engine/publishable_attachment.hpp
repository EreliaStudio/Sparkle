#pragma once

#include "design_pattern/publishable.hpp"
#include "exception.hpp"

#include <cstdint>
#include <string>

namespace spk
{
	class Entity;

	template <typename TAttachment>
	class PublishableAttachment : public TAttachment, public Publishable::Part
	{
	private:
		typename TAttachment::ActivationContract _activated;
		typename TAttachment::DeactivationContract _deactivated;

		void _encode(ByteStream::Writer &writer) const final
		{
			writer << static_cast<std::uint8_t>(this->isActive());
			_encodeData(writer);
		}

		void _decode(const ByteStream::Reader &reader) final
		{
			std::uint8_t active;
			reader >> active;
			if (active > 1)
			{
				throw spk::Exception("Invalid Publishable attachment activation state.");
			}
			_decodeData(reader);
			if (active != 0)
			{
				this->activate();
			}
			else
			{
				this->deactivate();
			}
		}

	protected:
		PublishableAttachment(const std::string &name, Entity *owner) :
			TAttachment(name, owner)
		{
			_activated = this->subscribeToActivation([this]() {
				this->invalidate();
			});
			_deactivated = this->subscribeToDeactivation([this]() {
				this->invalidate();
			});
		}

		virtual void _encodeData(ByteStream::Writer &) const
		{
		}
		virtual void _decodeData(const ByteStream::Reader &)
		{
		}
	};
}
