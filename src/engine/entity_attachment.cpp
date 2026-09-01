#include "engine/entity_attachment.hpp"

#include "engine/entity.hpp"

namespace spk
{
	EntityAttachment::EntityAttachment(
		const std::string &name,
		Entity *owner) :
		NameTrait(name)
	{
		activate();
		attach(owner);
	}

	EntityAttachment::EntityAttachment(Entity *owner) :
		EntityAttachment("Unnamed entity attachment", owner)
	{
	}

	void EntityAttachment::attach(Entity *owner)
	{
		if (_owner == owner)
		{
			return;
		}

		_onOwnerContextEditionContract.resign();
		_owner = owner;

		if (_owner == nullptr)
		{
			changeContext(nullptr);
			return;
		}

		changeContext(_owner->context());
		_onOwnerContextEditionContract = _owner->subscribeToContextEdition(
			[this](Engine *const &, Engine *const &newContext) {
				changeContext(newContext);
			});
	}

	Entity *EntityAttachment::owner()
	{
		return _owner;
	}

	const Entity *EntityAttachment::owner() const
	{
		return _owner;
	}
}
