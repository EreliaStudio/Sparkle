#pragma once

#include <functional>
#include "input/spk_input_state.hpp"
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
        Input(const TriggerCallback& p_trigger, const long long& p_triggerDelay, const Callback& p_callback);

        void update();

        void setTriggerDelay(const long long& p_triggerDelay);
        void setTriggerCallback(const TriggerCallback& p_trigger);
        void setCallback(const Callback& p_callback);

        Input toInput() const;
    };

    class KeyboardInput : public Input
    {
    private:
        using Input::setTriggerCallback;

    public:
        KeyboardInput(const spk::Keyboard::Key& p_key, const spk::InputState& p_expectedStatus, const long long& p_delay, const Callback& p_callback);

        void rebind(const spk::Keyboard::Key& p_key, const spk::InputState& p_expectedStatus);
    };

    class KeyboardCharInput : public Input
    {
    private:
        using Input::setTriggerCallback;
        using Input::setTriggerDelay;

    public:
        KeyboardCharInput(const Callback& p_callback);
    };

    class MouseInput : public Input
    {
    private:
        using Input::setTriggerCallback;

    public:
        MouseInput(const spk::Mouse::Button& p_button, const spk::InputState& p_expectedStatus, const long long& p_delay, const Callback& p_callback);

        void rebind(const spk::Mouse::Button& p_key, const spk::InputState& p_expectedStatus);
    };

    class MouseMotionInput : public Input
    {
    private:
        using Input::setTriggerCallback;
        using Input::setTriggerDelay;

    public:
        MouseMotionInput(const Callback& p_callback);
    };
}