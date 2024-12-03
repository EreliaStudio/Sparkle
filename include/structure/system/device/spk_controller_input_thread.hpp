#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "structure/thread/spk_persistant_worker.hpp"
#include <Windows.h>
#include <dinput.h>
#include "structure/system/device/spk_controller.hpp"
#include "structure/system/event/spk_event.hpp"

namespace spk
{
	/**
	 * @class ControllerInputThread
	 * @brief Manages input from a controller device in a separate thread.
	 *
	 * The `ControllerInputThread` class initializes and polls controller input using DirectInput
	 * and posts events based on input changes. It works in conjunction with a `PersistantWorker`
	 * to process controller states in a background thread.
	 *
	 * ### Example
	 * @code
	 * spk::ControllerInputThread controllerInputThread;
	 * controllerInputThread.bind(hWnd);
	 * controllerInputThread.start();
	 * // Perform other operations...
	 * controllerInputThread.stop();
	 * @endcode
	 */
	class ControllerInputThread
	{
	private:
		PersistantWorker _worker;
		HWND _hWnd = NULL;
		bool _initialization = false;
		IDirectInput8* _directInput = nullptr;
		IDirectInputDevice8* _controller = nullptr;
		DIJOYSTATE  _controllerState = DIJOYSTATE();
		DIJOYSTATE  _prevControllerState = DIJOYSTATE();

		/**
		 * @brief Initializes DirectInput and configures the controller device.
		 * @return True if initialization succeeded; otherwise, false.
		 */
		bool InitializeDirectInput();

		/**
		 * @brief Polls the controller for input states and posts relevant events.
		 */
		void PollController();

		/**
		 * @brief Posts a button press event.
		 * @param hWnd Handle to the application window.
		 * @param button The button that was pressed.
		 */
		static void PostButtonPress(HWND hWnd, spk::Controller::Button button);

		/**
		 * @brief Posts a button release event.
		 * @param hWnd Handle to the application window.
		 * @param button The button that was released.
		 */
		static void PostButtonRelease(HWND hWnd, spk::Controller::Button button);

		/**
		 * @brief Posts a left joystick movement event.
		 * @param hWnd Handle to the application window.
		 * @param p_x X-axis position of the joystick.
		 * @param p_y Y-axis position of the joystick.
		 */
		static void PostLeftJoystickMove(HWND hWnd, const unsigned short& p_x, const unsigned short& p_y);

		/**
		 * @brief Posts a right joystick movement event.
		 * @param hWnd Handle to the application window.
		 * @param p_x X-axis position of the joystick.
		 * @param p_y Y-axis position of the joystick.
		 */
		static void PostRightJoystickMove(HWND hWnd, const unsigned short& p_x, const unsigned short& p_y);

		/**
		 * @brief Posts a left trigger movement event.
		 * @param hWnd Handle to the application window.
		 * @param p_x Position of the left trigger.
		 */
		static void PostLeftTriggerMove(HWND hWnd, const int& p_x);

		/**
		 * @brief Posts a right trigger movement event.
		 * @param hWnd Handle to the application window.
		 * @param p_x Position of the right trigger.
		 */
		static void PostRightTriggerMove(HWND hWnd, const int& p_x);

		/**
		 * @brief Posts a directional cross movement event.
		 * @param hWnd Handle to the application window.
		 * @param p_x X-axis direction of the cross.
		 * @param p_y Y-axis direction of the cross.
		 */
		static void PostRightDirectionalCrossMotion(HWND hWnd, const int& p_x, const int& p_y);

	public:
		/**
		 * @brief Default constructor.
		 *
		 * Initializes the thread worker and sets up default values.
		 */
		ControllerInputThread();

		/**
		 * @brief Destructor.
		 *
		 * Releases any allocated resources and unacquires the controller.
		 */
		~ControllerInputThread();

		/**
		 * @brief Binds the thread to a specific application window.
		 * @param hWnd Handle to the application window.
		 */
		void bind(HWND hWnd);

		/**
		 * @brief Starts the controller input thread.
		 *
		 * Initializes DirectInput and begins polling controller input.
		 */
		void start();

		/**
		 * @brief Stops the controller input thread.
		 */
		void stop();
	};
}
