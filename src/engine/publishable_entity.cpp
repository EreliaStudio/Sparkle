#include "engine/publishable_entity.hpp"

#include "exception.hpp"

#include <limits>
#include <utility>

namespace spk
{
	PublishableEntity::ActivationPart::ActivationPart(Entity &entity) :
		_entity(entity)
	{
		_activated = _entity.subscribeToActivation([this](const auto &...) {
			invalidate();
		});
		_deactivated = _entity.subscribeToDeactivation([this](const auto &...) {
			invalidate();
		});
	}

	void PublishableEntity::ActivationPart::_encode(ByteStream::Writer &writer) const
	{
		writer << static_cast<std::uint8_t>(_entity.isActive());
	}

	void PublishableEntity::ActivationPart::_decode(const ByteStream::Reader &reader)
	{
		std::uint8_t active;
		reader >> active;
		if (active > 1)
		{
			throw spk::Exception("Invalid Entity activation state.");
		}
		if (active != 0)
		{
			_entity.activate();
		}
		else
		{
			_entity.deactivate();
		}
	}

	PublishableEntity::PublishableEntity(Mode mode) :
		Publishable(mode),
		_uuid(UUID::generate())
	{
		if (mode != Mode::Source)
		{
			throw spk::Exception("Source PublishableEntity requires Source mode.");
		}
	}

	PublishableEntity::PublishableEntity(Mode mode, const UUID &uuid) :
		Publishable(mode),
		_uuid(uuid)
	{
		if (mode != Mode::Replicant || uuid.isNull())
		{
			throw spk::Exception("Replicant PublishableEntity requires a non-null authoritative UUID.");
		}
	}

	void PublishableEntity::_install()
	{
		PartRegistry all;
		all.reserve(_internalParts.size() + _gameParts.size());
		for (const PartRegistration &registration : _internalParts)
		{
			all.push_back(registration);
		}
		for (const PartRegistration &registration : _gameParts)
		{
			all.push_back(registration);
		}
		Publishable::setPartRegistry(std::move(all));
	}

	void PublishableEntity::registerInternalPart(PartID id, Part &part, Flags<Publication> flags)
	{
		if (id >= FirstGamePartID)
		{
			throw spk::Exception("Internal PublishableEntity Part ID must be reserved.");
		}
		for (const PartRegistration &registered : _internalParts)
		{
			if (registered.id == id)
			{
				throw spk::Exception("Duplicate internal PublishableEntity Part ID.");
			}
		}
		PartRegistry replacement = _internalParts;
		replacement.push_back({id, part, flags});
		_internalParts.swap(replacement);
		try
		{
			_install();
		} catch (...)
		{
			_internalParts.pop_back();
			throw;
		}
	}

	void PublishableEntity::setPartRegistry(PartRegistry registry)
	{
		PartRegistry translated;
		translated.reserve(registry.size());
		for (const PartRegistration &registration : registry)
		{
			if (registration.id > std::numeric_limits<PartID>::max() - FirstGamePartID)
			{
				throw spk::Exception("PublishableEntity game Part ID overflow.");
			}
			translated.push_back({registration.id + FirstGamePartID, registration.part, registration.flags});
		}
		PartRegistry previous = _gameParts;
		_gameParts.swap(translated);
		try
		{
			_install();
		} catch (...)
		{
			_gameParts.swap(previous);
			throw;
		}
	}

	bool PublishableEntity::_decodeLast(PartID id) const
	{
		return id == 0;
	}

	const UUID &PublishableEntity::uuid() const noexcept
	{
		return _uuid;
	}
}
