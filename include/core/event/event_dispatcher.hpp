#pragma once

#include <functional>

#include "core/event/record.hpp"

namespace spk
{
class EventDispatcher
{
protected:
	virtual bool _isAcceptingInteraction() const = 0;

	virtual void _propagateInteraction(
		const std::function<void(EventDispatcher *)> &callback)
	{
	}

	virtual void _onWindowResizedEvent(WindowResizedEvent &event);
	virtual void _onWindowMovedEvent(WindowMovedEvent &event);
	virtual void _onWindowFocusGainedEvent(WindowFocusGainedEvent &event);
	virtual void _onWindowFocusLostEvent(WindowFocusLostEvent &event);

	virtual void _onMouseEnteredEvent(MouseEnteredEvent &event);
	virtual void _onMouseLeftEvent(MouseLeftEvent &event);
	virtual void _onMouseMovedEvent(MouseMovedEvent &event);
	virtual void _onMouseWheelScrolledEvent(MouseWheelScrolledEvent &event);
	virtual void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event);
	virtual void _onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event);
	virtual void _onMouseButtonDoubleClickedEvent(
		MouseButtonDoubleClickedEvent &event);

	virtual void _onKeyPressedEvent(KeyPressedEvent &event);
	virtual void _onKeyReleasedEvent(KeyReleasedEvent &event);
	virtual void _onTextInputEvent(TextInputEvent &event);

	virtual void _onPassiveMouseMovedEvent(MouseMovedEvent &event);
	virtual void _onPassiveMouseButtonPressedEvent(
		MouseButtonPressedEvent &event);
	virtual void _onPassiveKeyPressedEvent(KeyPressedEvent &event);
	virtual void _onPassiveKeyReleasedEvent(KeyReleasedEvent &event);

public:
	virtual ~EventDispatcher() = default;

	void dispatch(WindowResizedEvent &event);
	void dispatch(WindowMovedEvent &event);
	void dispatch(WindowFocusGainedEvent &event);
	void dispatch(WindowFocusLostEvent &event);

	void dispatch(MouseEnteredEvent &event);
	void dispatch(MouseLeftEvent &event);
	void dispatch(MouseMovedEvent &event);
	void dispatch(MouseWheelScrolledEvent &event);
	void dispatch(MouseButtonPressedEvent &event);
	void dispatch(MouseButtonReleasedEvent &event);
	void dispatch(MouseButtonDoubleClickedEvent &event);

	void dispatch(KeyPressedEvent &event);
	void dispatch(KeyReleasedEvent &event);
	void dispatch(TextInputEvent &event);

	void observePointer(MouseMovedEvent &event);
	void observePointer(MouseButtonPressedEvent &event);
	void observeKeyboard(KeyPressedEvent &event);
	void observeKeyboard(KeyReleasedEvent &event);
};	
}