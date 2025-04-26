#include "application/module/spk_update_module.hpp"

#include "structure/graphics/spk_window.hpp"

namespace spk
{
	void UpdateModule::_treatEvent(spk::UpdateEvent &&p_event)
	{
		if (_lastSeconds == 0)
		{
			_lastSeconds = p_event.time.seconds;
		}
		if (_lastMilliseconds == 0)
		{
			_lastMilliseconds = p_event.time.milliseconds;
		}
		if (_lastNanoseconds == 0)
		{
			_lastNanoseconds = p_event.time.nanoseconds;
		}

		p_event.deltaTime.seconds = p_event.time.seconds - _lastSeconds;
		p_event.deltaTime.milliseconds = p_event.time.milliseconds - _lastMilliseconds;
		p_event.deltaTime.nanoseconds = p_event.time.nanoseconds - _lastNanoseconds;

		_lastSeconds = p_event.time.seconds;
		_lastMilliseconds = p_event.time.milliseconds;
		_lastNanoseconds = p_event.time.nanoseconds;

		if (_widget != nullptr)
		{
			_widget->onUpdateEvent(p_event);
		}
	}

	spk::UpdateEvent UpdateModule::_convertEventToEventType(spk::Event &&p_event)
	{
		return (p_event.updateEvent);
	}

	UpdateModule::UpdateModule(spk::SafePointer<spk::Widget> p_widget) :
		_widget(p_widget)
	{
	}
}