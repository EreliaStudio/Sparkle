#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#include <Windows.h>

#include "structure/spk_safe_pointer.hpp"
#include "structure/system/device/spk_mouse.hpp"
#include "structure/system/device/spk_keyboard.hpp"
#include "structure/system/device/spk_controller.hpp"

#include "structure/graphics/spk_geometry_2D.hpp"

#include <unordered_map>
#include <functional>
#include <chrono>

static const UINT WM_UPDATE_REQUEST = RegisterWindowMessage("WM_UPDATE_REQUEST");
static const UINT WM_PAINT_REQUEST = RegisterWindowMessage("WM_PAINT_REQUEST");
static const UINT WM_LEFT_JOYSTICK_MOTION = RegisterWindowMessage("WM_LEFT_JOYSTICK_MOTION");
static const UINT WM_RIGHT_JOYSTICK_MOTION = RegisterWindowMessage("WM_RIGHT_JOYSTICK_MOTION");
static const UINT WM_LEFT_TRIGGER_MOTION = RegisterWindowMessage("WM_LEFT_TRIGGER_MOTION");
static const UINT WM_RIGHT_TRIGGER_MOTION = RegisterWindowMessage("WM_RIGHT_TRIGGER_MOTION");
static const UINT WM_DIRECTIONAL_CROSS_MOTION = RegisterWindowMessage("WM_DIRECTIONAL_CROSS_MOTION");
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
		HWND _hwnd;

	public:
		mutable bool _consumed;
		Modifiers _modifiers;
		spk::SafePointer<spk::Window> window = nullptr;

		IEvent(HWND p_hwnd) :
			_hwnd(p_hwnd),
			_consumed(false),
			_modifiers()
		{}

		void consume() const
		{
			_consumed = true;
		}

		bool consumed() const
		{
			return (_consumed);
		}

		void requestPaint() const;
		void requestUpdate() const;
	};

	struct PaintEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = { WM_PAINT_REQUEST };
		enum class Type
		{
			Unknow,
			Requested
		};
		Type type = Type::Unknow;

		PaintEvent(HWND p_hwnd) :
			IEvent(p_hwnd)
		{

		}
	};

	struct UpdateEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = { WM_UPDATE_REQUEST };
		enum class Type
		{
			Unknow,
			Requested
		};
		Type type = Type::Unknow;
		long long time;
		long long epoch;
		long long deltaTime = 0;
		spk::SafePointer<const spk::Mouse> mouse = nullptr;
		spk::SafePointer<const spk::Keyboard> keyboard = nullptr;
		spk::SafePointer<const spk::Controller> controller = nullptr;

		UpdateEvent(HWND p_hwnd) :
			IEvent(p_hwnd),
			type(spk::UpdateEvent::Type::Unknow)
		{

		}
	};

	struct MouseEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = {
			WM_LBUTTONDOWN, WM_RBUTTONDOWN, WM_MBUTTONDOWN,
			WM_LBUTTONUP, WM_RBUTTONUP, WM_MBUTTONUP,
			WM_LBUTTONDBLCLK, WM_RBUTTONDBLCLK, WM_MBUTTONDBLCLK,
			WM_MOUSEMOVE, WM_MOUSEWHEEL
		};
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
		const spk::Mouse* mouse = nullptr;
		union
		{
			spk::Mouse::Button button;
			spk::Vector2Int position;
			float scrollValue;
		};

		MouseEvent(HWND p_hwnd) :
			IEvent(p_hwnd)
		{

		}
	};

	struct KeyboardEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = {
			WM_KEYDOWN, WM_KEYUP, WM_CHAR
		};
		enum class Type
		{
			Unknow,
			Press,
			Release,
			Glyph
		};
		Type type = Type::Unknow;
		const spk::Keyboard* keyboard = nullptr;
		union
		{
			spk::Keyboard::Key key;
			wchar_t glyph;
		};

		KeyboardEvent(HWND p_hwnd) :
			IEvent(p_hwnd)
		{

		}
	};

	struct ControllerEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = {
			WM_LEFT_JOYSTICK_MOTION,
			WM_RIGHT_JOYSTICK_MOTION,
			WM_LEFT_TRIGGER_MOTION,
			WM_RIGHT_TRIGGER_MOTION,
			WM_DIRECTIONAL_CROSS_MOTION,
			WM_CONTROLLER_BUTTON_PRESS,
			WM_CONTROLLER_BUTTON_RELEASE
		};
		enum class Type
		{
			Unknow,
			Press,
			Release,
			TriggerMotion,
			JoystickMotion,
			DirectionalCrossMotion
		};
		static Controller::Button apiValueToControllerButton(int value);

		Type type = Type::Unknow;
		const spk::Controller* controller;
		union
		{
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

		ControllerEvent(HWND p_hwnd) :
			IEvent(p_hwnd)
		{

		}
	};

	struct SystemEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = { 
			WM_SIZE,
			WM_SETFOCUS,
			WM_KILLFOCUS,
			WM_CLOSE,
			WM_QUIT,
			WM_MOVE,
			WM_ENTERSIZEMOVE,
			WM_EXITSIZEMOVE
		};
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

		SystemEvent(HWND p_hwnd) :
			IEvent(p_hwnd)
		{

		}
	};

	struct TimerEvent : public IEvent
	{
		static inline std::vector<UINT> EventIDs = { WM_TIMER };
		enum class Type
		{
			Unknow,
			Timer
		};
		Type type = Type::Unknow;
		int timerID;

		TimerEvent(HWND p_hwnd) :
			IEvent(p_hwnd),
			type(spk::TimerEvent::Type::Timer)
		{

		}
	};

	struct Event
	{
		using ConstructorLambda = std::function<void(Event* p_event, UINT uMsg, WPARAM wParam, LPARAM lParam)>;
		static const std::unordered_map<UINT, ConstructorLambda> _constructionMap;
	
		union
		{
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
		Event(spk::SafePointer<Window> p_window, UINT uMsg, WPARAM wParam, LPARAM lParam);

		bool construct(spk::SafePointer<Window> p_window, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void setModifiers(UINT uMsg);
	};
}

std::ostream& operator << (std::ostream& p_os, const spk::MouseEvent::Type& p_type);
std::wostream& operator << (std::wostream& p_os, const spk::MouseEvent::Type& p_type);

std::ostream& operator << (std::ostream& p_os, const spk::KeyboardEvent::Type& p_type);
std::wostream& operator << (std::wostream& p_os, const spk::KeyboardEvent::Type& p_type);

std::ostream& operator << (std::ostream& p_os, const spk::ControllerEvent::Type& p_type);
std::wostream& operator << (std::wostream& p_os, const spk::ControllerEvent::Type& p_type);

std::ostream& operator << (std::ostream& p_os, const spk::SystemEvent::Type& p_type);
std::wostream& operator << (std::wostream& p_os, const spk::SystemEvent::Type& p_type);

std::ostream& operator << (std::ostream& p_os, const spk::PaintEvent::Type& p_type);
std::wostream& operator << (std::wostream& p_os, const spk::PaintEvent::Type& p_type);

std::ostream& operator << (std::ostream& p_os, const spk::UpdateEvent::Type& p_type);
std::wostream& operator << (std::wostream& p_os, const spk::UpdateEvent::Type& p_type);

std::ostream& operator << (std::ostream& p_os, const spk::UpdateEvent::Type& p_type);
std::wostream& operator << (std::wostream& p_os, const spk::UpdateEvent::Type& p_type);