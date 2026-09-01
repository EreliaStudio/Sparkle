#include "engine/system.hpp"

#include "core/context/update_context.hpp"

namespace spk
{
	System::System(const std::string &name, Engine *engine) :
		NameTrait(name)
	{
		activate();
		attach(engine);
	}

	System::System(Engine *engine) :
		System("Unnamed system", engine)
	{
	}

	bool System::_isAcceptingInteraction() const
	{
		return isActive();
	}

	void System::_updateState(UpdateContext &)
	{
	}

	void System::attach(Engine *engine)
	{
		changeContext(engine);
	}

	Engine *System::engine()
	{
		return context();
	}

	const Engine *System::engine() const
	{
		return context();
	}

	void System::updateState(UpdateContext &context)
	{
		if (!isActive())
		{
			return;
		}

		_updateState(context);
	}
}
