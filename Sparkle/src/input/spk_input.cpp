#include "input/spk_input.hpp"
#include "application/spk_application.hpp"

namespace spk
{
	Input::Input(const TriggerCallback& p_trigger, const long long& p_triggerDelay, const Callback& p_callback) :
		_trigger(p_trigger),
		_timer(p_triggerDelay),
		_callback(p_callback)
	{
	}

	void Input::update()
	{
		if (_timer.isTimedOut() == true && _trigger() == true)
		{
			_callback();
			_timer.start();
		}
	}

	void Input::setTriggerDelay(const long long& p_triggerDelay)
	{
		_timer.setDuration(p_triggerDelay);
	}

	void Input::setTriggerCallback(const TriggerCallback& p_trigger)
	{
		_trigger = p_trigger;
	}

	void Input::setCallback(const Callback& p_callback)
	{
		_callback = p_callback;
	}

	Input Input::toInput() const
	{
		return (Input(_trigger, _timer.duration(), _callback));
	}

	KeyboardInput::KeyboardInput(const spk::Keyboard::Key& p_key, const spk::InputState& p_expectedStatus, const long long& p_delay, const Callback& p_callback) :
		Input([&, p_key, p_expectedStatus]()
			  { return (spk::Application::activeApplication()->keyboard().getKey(p_key) == p_expectedStatus); },
			  p_delay, p_callback)
	{

	}

	void KeyboardInput::rebind(const spk::Keyboard::Key& p_key, const spk::InputState& p_expectedStatus)
	{
		setTriggerCallback(
			[&, p_key, p_expectedStatus](){ return (spk::Application::activeApplication()->keyboard().getKey(p_key) == p_expectedStatus); });
	}

	KeyboardCharInput::KeyboardCharInput(const Callback& p_callback) :
		Input([&]()
			  { return (spk::Application::activeApplication()->keyboard().getChar() != L'\0'); },
			  0, p_callback)
	{

	}

	MouseInput::MouseInput(const spk::Mouse::Button& p_button, const spk::InputState& p_expectedStatus, const long long& p_delay, const Callback& p_callback) :
		Input([&, p_button, p_expectedStatus](){ return (spk::Application::activeApplication()->mouse().getButton(p_button) == p_expectedStatus); },
			  p_delay, p_callback)
	{

	}

	void MouseInput::rebind(const spk::Mouse::Button& p_button, const spk::InputState& p_expectedStatus)
	{
		setTriggerCallback(
			[&, p_button, p_expectedStatus](){ return (spk::Application::activeApplication()->mouse().getButton(p_button) == p_expectedStatus); });
	}

	MouseMotionInput::MouseMotionInput(const Callback& p_callback) :
		Input([&]()
			  { return (spk::Application::activeApplication()->mouse().deltaPosition() != spk::Vector2Int(0, 0)); },
			  0, p_callback)
	{

	}
}