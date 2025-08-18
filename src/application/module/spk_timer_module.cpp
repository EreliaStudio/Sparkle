#include "application/module/spk_timer_module.hpp"

#include "structure/graphics/spk_window.hpp"

namespace spk
{
	void TimerModule::_treatEvent(spk::TimerEvent p_event)
	{
		if (p_event.timerID == UpdaterIdentifier)
		{
			p_event.window->requestUpdate();
		}
		else
		{
			p_event.timerID -= 2;
			p_event.window->widget()->onTimerEvent(p_event);
		}
	}

	spk::TimerEvent TimerModule::_convertEventToEventType(spk::Event p_event)
	{
		return (p_event.timerEvent);
	}
}