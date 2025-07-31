#include "structure/engine/spk_action.hpp"

namespace spk
{
	MouseMotionAction::MouseMotionAction(Mode p_mode, const std::function<void(const spk::Vector2Int&)>& p_callback) :
		_onTriggerCallback(p_callback),
		_mode(p_mode),
		_mouse(nullptr)
	{
	}

	bool MouseMotionAction::isInitialized() const
	{
		return (_mouse != nullptr);
	}

	void MouseMotionAction::initialize(spk::UpdateEvent& p_event)
	{
		_mouse = p_event.mouse;
	}

	spk::SafePointer<const spk::Mouse> MouseMotionAction::mouse() const
	{
		return (_mouse);
	}

	void MouseMotionAction::setMouse(spk::SafePointer<const spk::Mouse> p_mouse)
	{
		_mouse = p_mouse;
	}

	void MouseMotionAction::update()
	{
		if (_mouse == nullptr)
		{
			GENERATE_ERROR("Can't update an InputAction without device");
		}

		_onTriggerCallback(_mode == Mode::Absolute ? _mouse->position : _mouse->deltaPosition);
	}
}
