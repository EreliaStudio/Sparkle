#pragma once

#include "structure/engine/spk_component.hpp"

#include "event.hpp"

class ControlMapper : public spk::Component
{
private:
	EventCenter::Instanciator _eventCenterInstanciator;

	std::vector<std::function<Event(const spk::ControllerEvent&)>> _controllerControls;
	std::vector<std::function<Event(const spk::KeyboardEvent&)>> _keyboardControls;
	std::vector<std::function<Event(const spk::MouseEvent&)>> _mouseControls;

public:
	ControlMapper(const std::wstring& p_name);

	void onControllerEvent(spk::ControllerEvent& p_event) override;
	void onKeyboardEvent(spk::KeyboardEvent& p_event) override;
	void onMouseEvent(spk::MouseEvent& p_event) override;
	void onUpdateEvent(spk::UpdateEvent& p_event) override;
};