#pragma once

#include "input/spk_keyboard.hpp"
#include "input/spk_mouse.hpp"
#include "system/spk_timer.hpp"

namespace spk
{
    class Input
    {
    public:
        using TriggerCallback = std::function<bool()>;
        using Callback = std::function<void()>;

    private:
        TriggerCallback _trigger;
        spk::Timer _timer;
        Callback _callback;

    public:
        Input(const TriggerCallback& p_trigger, const long long& p_triggerDelay, const Callback& p_callback) :
            _trigger(p_trigger),
            _timer(p_triggerDelay),
            _callback(p_callback)
        {

        }

        void update()
        {
            if (_timer.isTimedOut() == true && _trigger() == true)
            {
                _callback();
                _timer.start();
            }
        }

        void setTriggerDelay(const long long& p_triggerDelay)
        {
            _timer.setDuration(p_triggerDelay);
        }

        void setTriggerCallback(const TriggerCallback& p_trigger)
        {
            _trigger = p_trigger;
        }

        void setCallback(const Callback& p_callback)
        {
            _callback = p_callback;
        }
    };

    class KeyboardInput : public Input
    {
    private:
        using Input::setTriggerCallback;

    public:
        KeyboardInput(const spk::Keyboard::Key& p_key, const spk::InputStatus& p_expectedStatus, const long long& p_delay, const Callback& p_callback) :
            Input([&, p_key, p_expectedStatus](){
                return (spk::Application::activeApplication()->keyboard().getKey(p_key) == p_expectedStatus);
            }, p_delay, p_callback)
        {

        }

        void rebind(const spk::Keyboard::Key& p_key, const spk::InputStatus& p_expectedStatus)
        {
            setTriggerCallback([&, p_key, p_expectedStatus](){
                return (spk::Application::activeApplication()->keyboard().getKey(p_key) == p_expectedStatus);
            })
        }
    }

    class MouseInput : public Input
    {
    private:
        using Input::setTriggerCallback;

    public:
        MouseInput(const spk::Mouse::Button& p_button, const spk::InputStatus& p_expectedStatus, const long long& p_delay, const Callback& p_callback) :
            Input([&, p_button, p_expectedStatus](){
                return (spk::Application::activeApplication()->mouse().getButton(p_button) == p_expectedStatus);
            }, p_delay, p_callback)
        {

        }

        void rebind(const spk::Mouse::Button& p_key, const spk::InputStatus& p_expectedStatus)
        {
            setTriggerCallback([&, p_button, p_expectedStatus](){
                return (spk::Application::activeApplication()->mouse().getButton(p_button) == p_expectedStatus);
            })
        }
    }

    class MouseMotionInput : public Input
    {
    private:
        using Input::setTriggerCallback;
        using Input::setTriggerDelay;

    public:
        MouseMotionInput(const Callback& p_callback) :
            Input([&](){
                return (spk::Application::activeApplication()->mouse().deltaPosition() != spk::Vector2Int(0, 0));
            }, 0, p_callback)
        {

        }
    }
}