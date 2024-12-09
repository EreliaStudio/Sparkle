#include "application/module/spk_update_module.hpp"

#include "structure/graphics/spk_window.hpp"

namespace spk
{
	void UpdateModule::_treatEvent(spk::UpdateEvent&& p_event)
	{
		if (_lastTime == 0)
		{
			_lastTime = p_event.time;
		}

		p_event.deltaTime = p_event.time - _lastTime;

		_lastTime = p_event.time;

		p_event.window->widget()->onUpdateEvent(p_event);
	}

	spk::UpdateEvent UpdateModule::_convertEventToEventType(spk::Event&& p_event)
	{
		return (p_event.updateEvent);
	}

	UpdateModule::UpdateModule()
	{

	}
}