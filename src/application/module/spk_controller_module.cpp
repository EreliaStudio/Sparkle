#include "application/module/spk_controller_module.hpp"

#include "spk_debug_macro.hpp"
#include "structure/graphics/spk_window.hpp"
#include "structure/system/spk_exception.hpp"

#include <cstdint>
#include <cmath>
#include <limits>

namespace
{
    inline bool inDeadZone2D(const spk::Vector2Int& p_vector, int p_threshold)
    {
        return (std::abs(p_vector.x) <= p_threshold) && (std::abs(p_vector.y) <= p_threshold);
    }

    inline bool inDeadZone1D(float p_value, float p_threshold)
    {
        return (p_value > -p_threshold) && (p_value < p_threshold);
    }

    inline float normalizeTrigger(float p_raw)
    {
        constexpr float kMax = static_cast<float>(std::numeric_limits<std::uint16_t>::max());
        return p_raw / kMax;
    }
}

namespace spk
{
    // Small, focused handlers to keep _treatEvent simple
    void ControllerModule::_handleJoystickMotion(const ControllerEvent& p_event)
    {
        if (p_event.joystick.id == Controller::Joystick::ID::Left)
        {
            _controller._leftJoystick.delta    = p_event.joystick.values - _controller._leftJoystick.position;
            _controller._leftJoystick.position = p_event.joystick.values;

            if (inDeadZone2D(p_event.joystick.values, 20))
            {
                spk::ControllerInputThread::PostLeftJoystickReset(p_event.window->_hwnd);
            }
        }
        else if (p_event.joystick.id == Controller::Joystick::ID::Right)
        {
            _controller._rightJoystick.delta    = p_event.joystick.values - _controller._rightJoystick.position;
            _controller._rightJoystick.position = p_event.joystick.values;

            // FIX: check absolute components, not abs(bool)
            if (inDeadZone2D(p_event.joystick.values, 20))
            {
                spk::ControllerInputThread::PostRightJoystickReset(p_event.window->_hwnd);
            }
        }
    }

    void ControllerModule::_handleTriggerMotion(const ControllerEvent& p_event)
    {
        // NOTE: spk_event.hpp shows trigger.value is float.
        // Remove redundant casts; normalize via uint16 max for tidiness.
        float ratio = normalizeTrigger(p_event.trigger.value);

        if (p_event.trigger.id == Controller::Trigger::ID::Left)
        {
            _controller._leftTrigger.ratio = ratio;

            if (inDeadZone1D(p_event.trigger.value, 100.0f))
            {
                spk::ControllerInputThread::PostLeftTriggerReset(p_event.window->_hwnd);
            }
        }
        else if (p_event.trigger.id == Controller::Trigger::ID::Right)
        {
            _controller._rightTrigger.ratio = ratio;

            if (inDeadZone1D(p_event.trigger.value, 100.0f))
            {
                spk::ControllerInputThread::PostRightTriggerReset(p_event.window->_hwnd);
            }
        }
    }

    void ControllerModule::_handleDirectionalCrossMotion(const ControllerEvent& p_event)
    {
        _controller._directionalCross = p_event.directionalCross.values;

        // Likely intended: dead-zone on both axes (fix previous || and missing abs)
        if (inDeadZone2D(p_event.directionalCross.values, 20))
        {
            spk::ControllerInputThread::PostDirectionalCrossReset(p_event.window->_hwnd);
        }
    }

    void ControllerModule::_handlePress(const ControllerEvent& p_event)
    {
        if (p_event.button != Controller::Button::Unknow)
        {
            _controller._buttons[static_cast<int>(p_event.button)] = InputState::Down;
        }
    }

    void ControllerModule::_handleRelease(const ControllerEvent& p_event)
    {
        if (p_event.button != Controller::Button::Unknow)
        {
            _controller._buttons[static_cast<int>(p_event.button)] = InputState::Up;
        }
    }

    void ControllerModule::_handleJoystickReset(const ControllerEvent& p_event)
    {
        if (p_event.joystick.id == Controller::Joystick::ID::Left)
        {
            _controller._leftJoystick.delta = 0;
            _controller._leftJoystick.position = 0;
        }
        else if (p_event.joystick.id == Controller::Joystick::ID::Right)
        {
            _controller._rightJoystick.delta = 0;
            _controller._rightJoystick.position = 0;
        }
    }

    void ControllerModule::_handleTriggerReset(const ControllerEvent& p_event)
    {
        if (p_event.trigger.id == Controller::Trigger::ID::Left)
        {
            _controller._leftTrigger.ratio = 0.0f;
        }
        else if (p_event.trigger.id == Controller::Trigger::ID::Right)
        {
            _controller._rightTrigger.ratio = 0.0f;
        }
    }

    void ControllerModule::_handleDirectionalCrossReset()
    {
        _controller._directionalCross = 0;
    }

    // ─────────────────────────────────────────────────────────────────────────────

    void ControllerModule::_treatEvent(ControllerEvent p_event)
    {
        _controller._window = p_event.window;

        // Dispatch to small helpers to reduce cognitive complexity
        switch (p_event.type)
        {
            case ControllerEvent::Type::JoystickMotion:         _handleJoystickMotion(p_event); break;
            case ControllerEvent::Type::TriggerMotion:          _handleTriggerMotion(p_event); break;
            case ControllerEvent::Type::DirectionalCrossMotion: _handleDirectionalCrossMotion(p_event); break;
            case ControllerEvent::Type::Press:                  _handlePress(p_event); break;
            case ControllerEvent::Type::Release:                _handleRelease(p_event); break;
            case ControllerEvent::Type::JoystickReset:          _handleJoystickReset(p_event); break;
            case ControllerEvent::Type::TriggerReset:           _handleTriggerReset(p_event); break;
            case ControllerEvent::Type::DirectionalCrossReset:  _handleDirectionalCrossReset(); break;
            default:
                GENERATE_ERROR("Unknow ControllerEvent type received");
        }

        p_event.controller = &_controller;

        if (spk::Widget::focusedWidget(Widget::FocusType::ControllerFocus) != nullptr)
        {
            spk::Widget::focusedWidget(Widget::FocusType::ControllerFocus)->onControllerEvent(p_event);
        }
        else
        {
            p_event.window->widget()->onControllerEvent(p_event);
        }
    }

    ControllerEvent ControllerModule::_convertEventToEventType(spk::Event p_event)
    {
        return p_event.controllerEvent;
    }

    const spk::Controller& ControllerModule::controller() const
    {
        return _controller;
    }
} // namespace spk
