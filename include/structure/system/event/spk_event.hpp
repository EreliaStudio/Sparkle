#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

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

	/**
	 * @struct Modifiers
	 * @brief Represents the state of modifier keys (Control, Alt, and Shift).
	 *
	 * The `Modifiers` struct is used to track the state of modifier keys during an event, such as whether
	 * the Control, Alt, or Shift key is currently pressed.
	 *
	 * ### Example
	 * @code
	 * spk::Modifiers modifiers;
	 * modifiers.control = true; // Control key is pressed.
	 * modifiers.alt = false;	// Alt key is not pressed.
	 * modifiers.shift = true;   // Shift key is pressed.
	 * @endcode
	 */
	struct Modifiers
	{
		bool control = false; ///< Indicates if the Control key is pressed.
		bool alt = false;	 ///< Indicates if the Alt key is pressed.
		bool shift = false;   ///< Indicates if the Shift key is pressed.
	};

/**
 * @struct IEvent
 * @brief Base class for all events in the system.
 *
 * The `IEvent` struct provides a common interface for handling events, including functionality
 * to manage consumption status, associated modifiers, and window-specific requests.
 *
 * ### Example
 * @code
 * spk::IEvent event(hwnd);
 * event.consume(); // Mark the event as consumed.
 * if (!event.consumed())
 * {
 *	 // Process the event if it has not been consumed.
 * }
 * event.requestPaint(); // Request a repaint of the associated window.
 * @endcode
 */
	struct IEvent
	{
	private:
		HWND _hwnd;
		mutable bool _consumed;

	public:
		Modifiers modifiers;
		spk::SafePointer<spk::Window> window = nullptr;

		/**
		 * @brief Constructs an `IEvent` with a specified window handle.
		 * @param p_hwnd Handle to the originating window.
		 */
		IEvent(HWND p_hwnd);

		/**
		 * @brief Marks the event as consumed.
		 *
		 * Once consumed, the event should not be processed further.
		 */
		void consume() const;

		/**
		 * @brief Checks if the event has been consumed.
		 * @return True if the event is consumed; otherwise, false.
		 */
		bool consumed() const;

		/**
		 * @brief Requests a repaint for the associated window.
		 *
		 * The repaint operation is typically handled by the windowing system.
		 */
		void requestPaint() const;

		/**
		 * @brief Requests an update for the associated window.
		 *
		 * An update operation may involve recalculating or refreshing content.
		 */
		void requestUpdate() const;
	};

	/**
	 * @struct PaintEvent
	 * @brief Represents a paint event for rendering updates in a window.
	 *
	 * The `PaintEvent` is a specialized event derived from `IEvent` and is triggered
	 * when a repaint operation is requested for the associated window.
	 *
	 * ### Example
	 * @code
	 * spk::PaintEvent event(hwnd);
	 * event.type = spk::PaintEvent::Type::Requested; // Set the type of paint event.
	 * if (!event.consumed())
	 * {
	 *	 // Handle the paint event.
	 * }
	 * @endcode
	 */
	struct PaintEvent : public IEvent
	{
		/**
		 * @brief List of message IDs that correspond to paint events.
		 */
		static inline std::vector<UINT> EventIDs = { WM_PAINT_REQUEST };

		/**
		 * @enum Type
		 * @brief Enumerates the possible types of paint events.
		 */
		enum class Type
		{
			Unknow,	///< Unknown paint event type.
			Requested  ///< A paint request event.
		};

		Type type = Type::Unknow; ///< The type of the paint event.

		/**
		 * @brief Constructs a `PaintEvent` with a specified window handle.
		 * @param p_hwnd Handle to the originating window.
		 */
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
		spk::SafePointer<const spk::Mouse> mouse = nullptr;
		spk::SafePointer<const spk::Keyboard> keyboard = nullptr;
		spk::SafePointer<const spk::Controller> controller = nullptr;

		UpdateEvent(HWND p_hwnd) :
			IEvent(p_hwnd),
			type(spk::UpdateEvent::Type::Requested),
			time(duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())
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