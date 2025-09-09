#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <Windows.h>

#include "structure/spk_safe_pointer.hpp"
#include "structure/system/device/spk_controller.hpp"
#include "structure/system/device/spk_keyboard.hpp"
#include "structure/system/device/spk_mouse.hpp"

#include "structure/graphics/spk_geometry_2d.hpp"

#include <functional>
#include <unordered_map>

#include "utils/spk_system_utils.hpp"

static const UINT WM_UPDATE_REQUEST = RegisterWindowMessage("WM_UPDATE_REQUEST");
static const UINT WM_PAINT_REQUEST = RegisterWindowMessage("WM_PAINT_REQUEST");
static const UINT WM_RESIZE_REQUEST = RegisterWindowMessage("WM_RESIZE_REQUEST");
static const UINT WM_LEFT_JOYSTICK_MOTION = RegisterWindowMessage("WM_LEFT_JOYSTICK_MOTION");
static const UINT WM_RIGHT_JOYSTICK_MOTION = RegisterWindowMessage("WM_RIGHT_JOYSTICK_MOTION");
static const UINT WM_LEFT_JOYSTICK_RESET = RegisterWindowMessage("WM_LEFT_JOYSTICK_RESET");
static const UINT WM_RIGHT_JOYSTICK_RESET = RegisterWindowMessage("WM_RIGHT_JOYSTICK_RESET");
static const UINT WM_LEFT_TRIGGER_MOTION = RegisterWindowMessage("WM_LEFT_TRIGGER_MOTION");
static const UINT WM_RIGHT_TRIGGER_MOTION = RegisterWindowMessage("WM_RIGHT_TRIGGER_MOTION");
static const UINT WM_LEFT_TRIGGER_RESET = RegisterWindowMessage("WM_LEFT_TRIGGER_RESET");
static const UINT WM_RIGHT_TRIGGER_RESET = RegisterWindowMessage("WM_RIGHT_TRIGGER_RESET");
static const UINT WM_DIRECTIONAL_CROSS_MOTION = RegisterWindowMessage("WM_DIRECTIONAL_CROSS_MOTION");
static const UINT WM_DIRECTIONAL_CROSS_RESET = RegisterWindowMessage("WM_DIRECTIONAL_CROSS_RESET");
static const UINT WM_CONTROLLER_BUTTON_PRESS = RegisterWindowMessage("WM_CONTROLLER_BUTTON_PRESS");
static const UINT WM_CONTROLLER_BUTTON_RELEASE = RegisterWindowMessage("WM_CONTROLLER_BUTTON_RELEASE");

namespace spk
{
	class Window;

	struct Modifiers
	{
		bool control = false;
		bool alt = false;
		bool shift = false;
	};

	struct IEvent
	{
	private:
	public:
		bool consumed;
		Modifiers modifiers;
		spk::SafePointer<spk::Window> window = nullptr;

		IEvent() :
			consumed(false),
			modifiers()
		{
		}

		void consume()
		{
			consumed = true;
		}

		void requestPaint();
		void requestUpdate();
	};

	struct PaintEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = {WM_PAINT_REQUEST, WM_RESIZE_REQUEST};
		enum class Type
		{
			Unknow,
			Paint,
			Resize
		};
		Type type = Type::Unknow;
		spk::Geometry2D geometry;
	};

	struct UpdateEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = {WM_UPDATE_REQUEST};
		enum class Type
		{
			Unknow,
			Requested
		};
		Type type = Type::Unknow;
		Timestamp time;
		Duration deltaTime;

		spk::SafePointer<const spk::Mouse> mouse = nullptr;
		spk::SafePointer<const spk::Keyboard> keyboard = nullptr;
		spk::SafePointer<const spk::Controller> controller = nullptr;
	};

	struct MouseEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = {WM_LBUTTONDOWN,
													WM_RBUTTONDOWN,
													WM_MBUTTONDOWN,
													WM_LBUTTONUP,
													WM_RBUTTONUP,
													WM_MBUTTONUP,
													WM_LBUTTONDBLCLK,
													WM_RBUTTONDBLCLK,
													WM_MBUTTONDBLCLK,
													WM_MOUSEMOVE,
													WM_MOUSEWHEEL};
		enum class Type
		{
			Unknow,
			Press,
			Release,
			DoubleClick,
			Motion,
			Wheel
		};
		Type type = Type::Unknow;
		spk::SafePointer<const spk::Mouse> mouse = nullptr;
		union {
			spk::Mouse::Button button;
			spk::Vector2Int position;
			float scrollValue;
		};
	};

	struct KeyboardEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = {WM_KEYDOWN, WM_KEYUP, WM_CHAR};
		enum class Type
		{
			Unknow,
			Press,
			Release,
			Repeat,
			Glyph
		};
		Type type = Type::Unknow;
		const spk::Keyboard *keyboard = nullptr;
		union {
			spk::Keyboard::Key key;
			wchar_t glyph;
		};
	};

	struct ControllerEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = {WM_LEFT_JOYSTICK_MOTION,
													WM_RIGHT_JOYSTICK_MOTION,
													WM_LEFT_JOYSTICK_RESET,
													WM_RIGHT_JOYSTICK_RESET,
													WM_LEFT_TRIGGER_MOTION,
													WM_RIGHT_TRIGGER_MOTION,
													WM_LEFT_TRIGGER_RESET,
													WM_RIGHT_TRIGGER_RESET,
													WM_DIRECTIONAL_CROSS_MOTION,
													WM_DIRECTIONAL_CROSS_RESET,
													WM_CONTROLLER_BUTTON_PRESS,
													WM_CONTROLLER_BUTTON_RELEASE};
		enum class Type
		{
			Unknow,
			Press,
			Release,
			TriggerMotion,
			JoystickMotion,
			DirectionalCrossMotion,
			TriggerReset,
			JoystickReset,
			DirectionalCrossReset
		};
		static Controller::Button apiValueToControllerButton(int p_value);

		Type type = Type::Unknow;
		const spk::Controller *controller;
		union {
			spk::Controller::Button button;
			struct
			{
				spk::Controller::Joystick::ID id;
				spk::Vector2Int values;
			} joystick;
			struct
			{
				spk::Controller::Trigger::ID id;
				float value;
			} trigger;
			struct
			{
				spk::Vector2Int values;
			} directionalCross;
		};
	};

	struct SystemEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = {
			WM_SIZE, WM_SETFOCUS, WM_KILLFOCUS, WM_CLOSE, WM_QUIT, WM_MOVE, WM_ENTERSIZEMOVE, WM_EXITSIZEMOVE, WM_SETCURSOR};
		enum class Type
		{
			Unknow,
			EnterResize,
			Resize,
			ExitResize,
			TakeFocus,
			LoseFocus,
			Quit,
			Move
		};
		Type type = Type::Unknow;

		spk::Geometry2D::Size newSize;
		spk::Geometry2D::Point newPosition;
	};

	struct TimerEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = {WM_TIMER};
		enum class Type
		{
			Unknow,
			Timer
		};
		Type type = Type::Unknow;
		int timerID;
	};

	struct Event
	{
		using ConstructorLambda = std::function<void(Event *p_event, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)>;
		static const std::unordered_map<UINT, ConstructorLambda> _constructionMap;

		union {
			IEvent rootEvent;
			PaintEvent paintEvent;
			UpdateEvent updateEvent;
			MouseEvent mouseEvent;
			KeyboardEvent keyboardEvent;
			ControllerEvent controllerEvent;
			SystemEvent systemEvent;
			TimerEvent timerEvent;
		};

		Event();
		Event(spk::SafePointer<Window> p_window, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam);

		bool construct(spk::SafePointer<Window> p_window, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam);
		void setModifiers(UINT p_uMsg);
	};

	inline std::ostream &operator<<(std::ostream &p_os, const spk::MouseEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::MouseEvent::Type::Press:
			p_os << "Press";
			break;
		case spk::MouseEvent::Type::Release:
			p_os << "Release";
			break;
		case spk::MouseEvent::Type::DoubleClick:
			p_os << "DoubleClick";
			break;
		case spk::MouseEvent::Type::Motion:
			p_os << "Motion";
			break;
		case spk::MouseEvent::Type::Wheel:
			p_os << "Wheel";
			break;
		default:
			p_os << "Unknown";
			break;
		}
		return p_os;
	}

	inline std::wostream &operator<<(std::wostream &p_os, const spk::MouseEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::MouseEvent::Type::Press:
			p_os << L"Press";
			break;
		case spk::MouseEvent::Type::Release:
			p_os << L"Release";
			break;
		case spk::MouseEvent::Type::DoubleClick:
			p_os << L"DoubleClick";
			break;
		case spk::MouseEvent::Type::Motion:
			p_os << L"Motion";
			break;
		case spk::MouseEvent::Type::Wheel:
			p_os << L"Wheel";
			break;
		default:
			p_os << L"Unknown";
			break;
		}
		return p_os;
	}

	inline std::ostream &operator<<(std::ostream &p_os, const spk::KeyboardEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::KeyboardEvent::Type::Press:
			p_os << "Press";
			break;
		case spk::KeyboardEvent::Type::Release:
			p_os << "Release";
			break;
		case spk::KeyboardEvent::Type::Glyph:
			p_os << "Glyph";
			break;
		default:
			p_os << "Unknown";
			break;
		}
		return p_os;
	}

	inline std::wostream &operator<<(std::wostream &p_os, const spk::KeyboardEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::KeyboardEvent::Type::Press:
			p_os << L"Press";
			break;
		case spk::KeyboardEvent::Type::Release:
			p_os << L"Release";
			break;
		case spk::KeyboardEvent::Type::Glyph:
			p_os << L"Glyph";
			break;
		default:
			p_os << L"Unknown";
			break;
		}
		return p_os;
	}

	inline std::ostream &operator<<(std::ostream &p_os, const spk::ControllerEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::ControllerEvent::Type::Press:
			p_os << "Press";
			break;
		case spk::ControllerEvent::Type::Release:
			p_os << "Release";
			break;
		case spk::ControllerEvent::Type::JoystickMotion:
			p_os << "JoystickMotion";
			break;
		case spk::ControllerEvent::Type::TriggerMotion:
			p_os << "TriggerMotion";
			break;
		case spk::ControllerEvent::Type::DirectionalCrossMotion:
			p_os << "DirectionalCrossMotion";
			break;
		default:
			p_os << "Unknown";
			break;
		}
		return p_os;
	}

	inline std::wostream &operator<<(std::wostream &p_os, const spk::ControllerEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::ControllerEvent::Type::Press:
			p_os << L"Press";
			break;
		case spk::ControllerEvent::Type::Release:
			p_os << L"Release";
			break;
		case spk::ControllerEvent::Type::JoystickMotion:
			p_os << L"JoystickMotion";
			break;
		case spk::ControllerEvent::Type::TriggerMotion:
			p_os << L"TriggerMotion";
			break;
		case spk::ControllerEvent::Type::DirectionalCrossMotion:
			p_os << L"DirectionalCrossMotion";
			break;
		default:
			p_os << L"Unknown";
			break;
		}
		return p_os;
	}

	inline std::ostream &operator<<(std::ostream &p_os, const spk::SystemEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::SystemEvent::Type::Resize:
			p_os << "Resize";
			break;
		case spk::SystemEvent::Type::TakeFocus:
			p_os << "TakeFocus";
			break;
		case spk::SystemEvent::Type::LoseFocus:
			p_os << "LoseFocus";
			break;
		case spk::SystemEvent::Type::Quit:
			p_os << "Quit";
			break;
		case spk::SystemEvent::Type::Move:
			p_os << "Move";
			break;
		default:
			p_os << "Unknown";
			break;
		}
		return p_os;
	}

	inline std::wostream &operator<<(std::wostream &p_os, const spk::SystemEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::SystemEvent::Type::Resize:
			p_os << L"Resize";
			break;
		case spk::SystemEvent::Type::TakeFocus:
			p_os << L"TakeFocus";
			break;
		case spk::SystemEvent::Type::LoseFocus:
			p_os << L"LoseFocus";
			break;
		case spk::SystemEvent::Type::Quit:
			p_os << L"Quit";
			break;
		case spk::SystemEvent::Type::Move:
			p_os << L"Move";
			break;
		default:
			p_os << L"Unknown";
			break;
		}
		return p_os;
	}

	inline std::ostream &operator<<(std::ostream &p_os, const spk::PaintEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::PaintEvent::Type::Paint:
			p_os << "Paint";
			break;
		case spk::PaintEvent::Type::Resize:
			p_os << "Resize";
			break;
		default:
			p_os << "Unknown";
			break;
		}
		return p_os;
	}

	inline std::wostream &operator<<(std::wostream &p_os, const spk::PaintEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::PaintEvent::Type::Paint:
			p_os << L"Paint";
			break;
		case spk::PaintEvent::Type::Resize:
			p_os << L"Resize";
			break;
		default:
			p_os << L"Unknown";
			break;
		}
		return p_os;
	}

	inline std::ostream &operator<<(std::ostream &p_os, const spk::UpdateEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::UpdateEvent::Type::Requested:
			p_os << "Requested";
			break;
		default:
			p_os << "Unknown";
			break;
		}
		return p_os;
	}

	inline std::wostream &operator<<(std::wostream &p_os, const spk::UpdateEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::UpdateEvent::Type::Requested:
			p_os << L"Requested";
			break;
		default:
			p_os << L"Unknown";
			break;
		}
		return p_os;
	}

	inline std::ostream &operator<<(std::ostream &p_os, const spk::TimerEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::TimerEvent::Type::Timer:
			p_os << "Timer";
			break;
		default:
			p_os << "Unknown";
			break;
		}
		return p_os;
	}

	inline std::wostream &operator<<(std::wostream &p_os, const spk::TimerEvent::Type &p_type)
	{
		switch (p_type)
		{
		case spk::TimerEvent::Type::Timer:
			p_os << L"Timer";
			break;
		default:
			p_os << L"Unknown";
			break;
		}
		return p_os;
	}
}