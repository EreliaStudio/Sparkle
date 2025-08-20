#include "structure/system/event/spk_event.hpp"

#include "structure/graphics/spk_window.hpp"

#include "utils/spk_system_utils.hpp"

namespace spk
{
	void IEvent::requestPaint()
	{
		if (window != nullptr)
		{
			window->requestPaint();
		}
	}

	void IEvent::requestUpdate()
	{
		if (window != nullptr)
		{
			window->requestUpdate();
		}
	}

	Controller::Button ControllerEvent::apiValueToControllerButton(int p_value)
	{
		switch (p_value)
		{
		case 0:
			return Controller::Button::A;
		case 1:
			return Controller::Button::B;
		case 2:
			return Controller::Button::X;
		case 3:
			return Controller::Button::Y;
		case 4:
			return Controller::Button::LeftArrow;
		case 5:
			return Controller::Button::DownArrow;
		case 6:
			return Controller::Button::RightArrow;
		case 7:
			return Controller::Button::UpArrow;
		case 8:
			return Controller::Button::Start;
		case 9:
			return Controller::Button::Select;
		case 10:
			return Controller::Button::R1;
		case 11:
			return Controller::Button::L1;
		case 14:
			return Controller::Button::R3;
		case 15:
			return Controller::Button::L3;
		default:
			return Controller::Button::Unknow;
		}
	}

	Event::Event()
	{
	}

	Event::Event(spk::SafePointer<Window> p_window, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
	{
		rootEvent._consumed = false;
		construct(p_window, p_uMsg, p_wParam, p_lParam);
	}

	const std::unordered_map<UINT, Event::ConstructorLambda> Event::_constructionMap = {
		{WM_PAINT_REQUEST, [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam) { p_event->paintEvent.type = PaintEvent::Type::Paint; }},
		{WM_RESIZE_REQUEST,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->paintEvent.type = PaintEvent::Type::Resize;
			 p_event->paintEvent.geometry = {{0, 0}, spk::Vector2Int{static_cast<int>(p_wParam), static_cast<int>(p_lParam)}};
		 }},
		{WM_LBUTTONDOWN,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->mouseEvent.type = MouseEvent::Type::Press;
			 p_event->mouseEvent.button = spk::Mouse::Button::Left;
		 }},
		{WM_RBUTTONDOWN,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->mouseEvent.type = MouseEvent::Type::Press;
			 p_event->mouseEvent.button = spk::Mouse::Button::Right;
		 }},
		{WM_MBUTTONDOWN,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->mouseEvent.type = MouseEvent::Type::Press;
			 p_event->mouseEvent.button = spk::Mouse::Button::Middle;
		 }},
		{WM_LBUTTONUP,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->mouseEvent.type = MouseEvent::Type::Release;
			 p_event->mouseEvent.button = spk::Mouse::Button::Left;
		 }},
		{WM_RBUTTONUP,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->mouseEvent.type = MouseEvent::Type::Release;
			 p_event->mouseEvent.button = spk::Mouse::Button::Right;
		 }},
		{WM_MBUTTONUP,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->mouseEvent.type = MouseEvent::Type::Release;
			 p_event->mouseEvent.button = spk::Mouse::Button::Middle;
		 }},
		{WM_LBUTTONDBLCLK,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->mouseEvent.type = MouseEvent::Type::DoubleClick;
			 p_event->mouseEvent.button = spk::Mouse::Button::Left;
		 }},
		{WM_RBUTTONDBLCLK,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->mouseEvent.type = MouseEvent::Type::DoubleClick;
			 p_event->mouseEvent.button = spk::Mouse::Button::Right;
		 }},
		{WM_MBUTTONDBLCLK,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->mouseEvent.type = MouseEvent::Type::DoubleClick;
			 p_event->mouseEvent.button = spk::Mouse::Button::Middle;
		 }},
		{WM_MOUSEMOVE,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->mouseEvent.type = MouseEvent::Type::Motion;
			 p_event->mouseEvent.position = spk::Vector2Int{LOWORD(p_lParam), HIWORD(p_lParam)};
		 }},
		{WM_MOUSEWHEEL,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->mouseEvent.type = MouseEvent::Type::Wheel;
			 p_event->mouseEvent.scrollValue = GET_WHEEL_DELTA_WPARAM(p_wParam) / (float)WHEEL_DELTA;
		 }},
		{WM_KEYDOWN,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->keyboardEvent.type = KeyboardEvent::Type::Press;
			 p_event->keyboardEvent.key = static_cast<spk::Keyboard::Key>(p_wParam);
		 }},
		{WM_KEYUP,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->keyboardEvent.type = KeyboardEvent::Type::Release;
			 p_event->keyboardEvent.key = static_cast<spk::Keyboard::Key>(p_wParam);
		 }},
		{WM_CHAR,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->keyboardEvent.type = KeyboardEvent::Type::Glyph;
			 p_event->keyboardEvent.glyph = static_cast<wchar_t>(p_wParam);
		 }},
		{WM_MOVE,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->systemEvent.type = SystemEvent::Type::Move;
			 p_event->systemEvent.newPosition = spk::Geometry2D::Point(LOWORD(p_lParam), HIWORD(p_lParam));
		 }},
		{WM_SETFOCUS,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam) { p_event->systemEvent.type = SystemEvent::Type::TakeFocus; }},
		{WM_KILLFOCUS,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam) { p_event->systemEvent.type = SystemEvent::Type::LoseFocus; }},
		{WM_CLOSE, [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam) { p_event->systemEvent.type = SystemEvent::Type::Quit; }},
		{WM_QUIT, [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam) { p_event->systemEvent.type = SystemEvent::Type::Quit; }},
		{WM_ENTERSIZEMOVE,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam) { p_event->systemEvent.type = SystemEvent::Type::EnterResize; }},
		{WM_SIZE,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->systemEvent.type = SystemEvent::Type::Resize;
			 p_event->systemEvent.newSize = spk::Geometry2D::Size(LOWORD(p_lParam), HIWORD(p_lParam));
		 }},
		{WM_EXITSIZEMOVE,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam) { p_event->systemEvent.type = SystemEvent::Type::ExitResize; }},
		{WM_LEFT_JOYSTICK_MOTION,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 static const short halfValue = std::numeric_limits<unsigned short>::max() / 2;
			 p_event->controllerEvent.type = ControllerEvent::Type::JoystickMotion;
			 p_event->controllerEvent.joystick.id = Controller::Joystick::ID::Left;
			 p_event->controllerEvent.joystick.values = spk::Vector2Int(
				 static_cast<unsigned short>(p_wParam) - halfValue,
				 std::numeric_limits<unsigned short>::max() - static_cast<unsigned short>(p_lParam) - halfValue);
		 }},
		{WM_RIGHT_JOYSTICK_MOTION,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 static const short halfValue = std::numeric_limits<unsigned short>::max() / 2;
			 p_event->controllerEvent.type = ControllerEvent::Type::JoystickMotion;
			 p_event->controllerEvent.joystick.id = Controller::Joystick::ID::Right;
			 p_event->controllerEvent.joystick.values = spk::Vector2Int(
				 static_cast<unsigned short>(p_wParam) - halfValue,
				 std::numeric_limits<unsigned short>::max() - static_cast<unsigned short>(p_lParam) - halfValue);
		 }},
		{WM_LEFT_JOYSTICK_RESET,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 static const short halfValue = std::numeric_limits<unsigned short>::max() / 2;
			 p_event->controllerEvent.type = ControllerEvent::Type::JoystickReset;
			 p_event->controllerEvent.joystick.id = Controller::Joystick::ID::Left;
			 p_event->controllerEvent.joystick.values = 0;
		 }},
		{WM_RIGHT_JOYSTICK_RESET,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 static const short halfValue = std::numeric_limits<unsigned short>::max() / 2;
			 p_event->controllerEvent.type = ControllerEvent::Type::JoystickReset;
			 p_event->controllerEvent.joystick.id = Controller::Joystick::ID::Right;
			 p_event->controllerEvent.joystick.values = 0;
		 }},
		{WM_LEFT_TRIGGER_MOTION,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->controllerEvent.type = ControllerEvent::Type::TriggerMotion;
			 p_event->controllerEvent.trigger.id = Controller::Trigger::ID::Left;
			 p_event->controllerEvent.trigger.value = static_cast<unsigned short>(p_wParam);
		 }},
		{WM_RIGHT_TRIGGER_MOTION,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->controllerEvent.type = ControllerEvent::Type::TriggerMotion;
			 p_event->controllerEvent.trigger.id = Controller::Trigger::ID::Right;
			 p_event->controllerEvent.trigger.value = static_cast<unsigned short>(p_wParam);
		 }},
		{WM_DIRECTIONAL_CROSS_MOTION,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->controllerEvent.type = ControllerEvent::Type::DirectionalCrossMotion;
			 p_event->controllerEvent.directionalCross.values =
				 spk::Vector2Int(static_cast<short>(LOWORD(p_lParam)), static_cast<short>(HIWORD(p_lParam)));
		 }},
		{WM_CONTROLLER_BUTTON_PRESS,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->controllerEvent.type = ControllerEvent::Type::Press;
			 p_event->controllerEvent.button = ControllerEvent::apiValueToControllerButton(LOWORD(p_lParam));
		 }},
		{WM_CONTROLLER_BUTTON_RELEASE,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->controllerEvent.type = ControllerEvent::Type::Release;
			 p_event->controllerEvent.button = ControllerEvent::apiValueToControllerButton(LOWORD(p_lParam));
		 }},
		{WM_UPDATE_REQUEST,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->updateEvent.type = UpdateEvent::Type::Requested;

			 p_event->updateEvent.time = SystemUtils::getTime();
		 }},
		{WM_TIMER,
		 [](Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
		 {
			 p_event->timerEvent.type = TimerEvent::Type::Timer;

			 p_event->timerEvent.timerID = p_wParam;
		 }}};

	bool Event::construct(spk::SafePointer<Window> p_window, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
	{
		if (_constructionMap.contains(p_uMsg) == false)
		{
			return (false);
		}
		if (p_uMsg == WM_PAINT)
		{
			ValidateRect(p_window->_hwnd, nullptr);
		}

		setModifiers(p_uMsg);
		this->rootEvent.window = p_window;
		_constructionMap.at(p_uMsg)(this, p_uMsg, p_wParam, p_lParam);
		return (true);
	}

	void Event::setModifiers(UINT p_uMsg)
	{
		IEvent *currentEvent = nullptr;

		switch (p_uMsg)
		{
		case WM_PAINT:
			currentEvent = &paintEvent;
			break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
			currentEvent = &mouseEvent;
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_CHAR:
			currentEvent = &keyboardEvent;
			break;

		case WM_ENTERSIZEMOVE:
		case WM_EXITSIZEMOVE:
		case WM_SIZE:
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_CLOSE:
		case WM_QUIT:
		case WM_MOVE:
			currentEvent = &systemEvent;
			break;

		default:
			return;
		}

		if (currentEvent != nullptr)
		{
			currentEvent->_modifiers.control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
			currentEvent->_modifiers.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
			currentEvent->_modifiers.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
		}
	}
}