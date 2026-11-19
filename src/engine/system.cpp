#include "engine/system.hpp"

#include "core/context/update_context.hpp"

namespace spk
{
	System::System(const std::string &name, EngineHeadless *engine) :
		NameTrait(name)
	{
		activate();
		attach(engine);
	}

	System::System(EngineHeadless *engine) :
		System("Unnamed system", engine)
	{
	}

	bool System::_canUpdate() const
	{
		return isActive();
	}

	void System::attach(EngineHeadless *engine)
	{
		changeContext(engine);
	}

	EngineHeadless *System::engine()
	{
		return context();
	}

	const EngineHeadless *System::engine() const
	{
		return context();
	}
}
