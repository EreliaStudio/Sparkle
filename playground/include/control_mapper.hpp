#pragma once

#include "structure/engine/spk_component.hpp"

#include "event.hpp"

class ControlMapper : public spk::Component
{
private:
	EventCenter::Instanciator _eventCenterInstanciator;

	Event _motionEvent = Event::NoEvent;

public:
	ControlMapper(const std::wstring& p_name);

	void onControllerEvent(spk::ControllerEvent& p_event) override;
	void onKeyboardEvent(spk::KeyboardEvent& p_event) override;
	void onMouseEvent(spk::MouseEvent& p_event) override;
	void onUpdateEvent(spk::UpdateEvent& p_event) override;
};