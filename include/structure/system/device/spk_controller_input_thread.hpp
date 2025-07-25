#pragma once

#include "utils/spk_platform.hpp"

#ifdef _WIN32
#define DIRECTINPUT_VERSION 0x0800

#include "structure/math/spk_vector2.hpp"
#include "structure/spk_iostream.hpp"
#include "structure/system/device/spk_controller.hpp"
#include "structure/system/event/spk_event.hpp"
#include "structure/thread/spk_persistant_worker.hpp"
#include <dinput.h>
#include <iostream>

namespace spk
{
class ControllerInputThread
{
		friend class ControllerModule;

	private:
		PersistantWorker _worker;
		HWND _hWnd = NULL;
		bool _initialization = false;
		IDirectInput8 *_directInput = nullptr;
		IDirectInputDevice8 *_controller = nullptr;
		DIJOYSTATE _controllerState = DIJOYSTATE();
		DIJOYSTATE _prevControllerState = DIJOYSTATE();

		bool InitializeDirectInput()
		{
			HRESULT hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)&_directInput, NULL);
			if (FAILED(hr))
			{
				return false;
			}

			hr = _directInput->CreateDevice(GUID_Joystick, &_controller, NULL);
			if (FAILED(hr))
			{
				return false;
			}

			hr = _controller->SetDataFormat(&c_dfDIJoystick);
			if (FAILED(hr))
			{
				return false;
			}

			hr = _controller->SetCooperativeLevel(_hWnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
			if (FAILED(hr))
			{
				return false;
			}

			return true;
		}

		void PollController()
		{
			if (!_controller)
			{
				return;
			}

			HRESULT hr = _controller->Poll();
			if (FAILED(hr))
			{
				hr = _controller->Acquire();
				while (hr == DIERR_INPUTLOST)
				{
					hr = _controller->Acquire();
				}
				return;
			}

			hr = _controller->GetDeviceState(sizeof(DIJOYSTATE), &_controllerState);
			if (FAILED(hr))
			{
				return;
			}

			// Process buttons
			for (int i = 0; i < 32; ++i)
			{
				if ((_controllerState.rgbButtons[i] & 0x80) && !(_prevControllerState.rgbButtons[i] & 0x80))
				{
					PostButtonPress(_hWnd, static_cast<spk::Controller::Button>(i));
					_prevControllerState.rgbButtons[i] = _controllerState.rgbButtons[i];
				}
				else if (!(_controllerState.rgbButtons[i] & 0x80) && (_prevControllerState.rgbButtons[i] & 0x80))
				{
					PostButtonRelease(_hWnd, static_cast<spk::Controller::Button>(i));
					_prevControllerState.rgbButtons[i] = _controllerState.rgbButtons[i];
				}
			}

			if (_controllerState.lX != _prevControllerState.lX || _controllerState.lY != _prevControllerState.lY)
			{
				PostLeftJoystickMove(_hWnd, static_cast<unsigned short>(_controllerState.lX), static_cast<unsigned short>(_controllerState.lY));
				_prevControllerState.lX = _controllerState.lX;
				_prevControllerState.lY = _controllerState.lY;
			}

			if (_controllerState.lRx != _prevControllerState.lRx || _controllerState.lRy != _prevControllerState.lRy)
			{
				PostRightJoystickMove(_hWnd, static_cast<unsigned short>(_controllerState.lRx), static_cast<unsigned short>(_controllerState.lRy));
				_prevControllerState.lRx = _controllerState.lRx;
				_prevControllerState.lRy = _controllerState.lRy;
			}

			if (_controllerState.lZ != _prevControllerState.lZ)
			{
				unsigned short leftTriggerValue = (_controllerState.lZ & 0xFF00) >> 8; // Extract high byte for L2
				unsigned short rightTriggerValue = _controllerState.lZ & 0x00FF;	   // Extract low byte for R2

				unsigned short prevLeftTriggerValue = (_prevControllerState.lZ & 0xFF00) >> 8; // Extract high byte for L2
				unsigned short prevRightTriggerValue = _prevControllerState.lZ & 0x00FF;	   // Extract low byte for R2

				if (leftTriggerValue != prevLeftTriggerValue)
				{
					PostLeftTriggerMove(_hWnd, leftTriggerValue);
				}
				if (rightTriggerValue != prevRightTriggerValue)
				{
					PostRightTriggerMove(_hWnd, rightTriggerValue);
				}

				_prevControllerState.lZ = _controllerState.lZ;
			}

			if (_controllerState.rgdwPOV[0] != _prevControllerState.rgdwPOV[0])
			{
				switch (_controllerState.rgdwPOV[0])
				{
				case static_cast<long unsigned int>(-1):
					PostDirectionalCrossMotion(_hWnd, 0, 0);
					break;
				case 0:
					PostDirectionalCrossMotion(_hWnd, 0, 1);
					break;
				case 4500:
					PostDirectionalCrossMotion(_hWnd, 1, 1);
					break;
				case 9000:
					PostDirectionalCrossMotion(_hWnd, 1, 0);
					break;
				case 13500:
					PostDirectionalCrossMotion(_hWnd, 1, -1);
					break;
				case 18000:
					PostDirectionalCrossMotion(_hWnd, 0, -1);
					break;
				case 22500:
					PostDirectionalCrossMotion(_hWnd, -1, -1);
					break;
				case 27000:
					PostDirectionalCrossMotion(_hWnd, -1, 0);
					break;
				case 31500:
					PostDirectionalCrossMotion(_hWnd, -1, 1);
					break;
				}
				_prevControllerState.rgdwPOV[0] = _controllerState.rgdwPOV[0];
			}
		}

		static void PostButtonPress(HWND hWnd, spk::Controller::Button button)
		{
			PostMessage(hWnd, WM_CONTROLLER_BUTTON_PRESS, static_cast<int>(button), 0);
		}

		static void PostButtonRelease(HWND hWnd, spk::Controller::Button button)
		{
			PostMessage(hWnd, WM_CONTROLLER_BUTTON_RELEASE, static_cast<int>(button), 0);
		}

		static void PostLeftJoystickMove(HWND hWnd, const unsigned short &p_x, const unsigned short &p_y)
		{
			PostMessage(hWnd, WM_LEFT_JOYSTICK_MOTION, static_cast<WPARAM>(p_x), static_cast<LPARAM>(p_y));
		}

		static void PostRightJoystickMove(HWND hWnd, const unsigned short &p_x, const unsigned short &p_y)
		{
			PostMessage(hWnd, WM_RIGHT_JOYSTICK_MOTION, static_cast<WPARAM>(p_x), static_cast<LPARAM>(p_y));
		}

		static void PostLeftJoystickReset(HWND hWnd)
		{
			PostMessage(hWnd, WM_LEFT_JOYSTICK_RESET, 0, 0);
		}

		static void PostRightJoystickReset(HWND hWnd)
		{
			PostMessage(hWnd, WM_RIGHT_JOYSTICK_RESET, 0, 0);
		}

		static void PostLeftTriggerMove(HWND hWnd, const int &p_x)
		{
			PostMessage(hWnd, WM_LEFT_TRIGGER_MOTION, p_x, 0);
		}

		static void PostRightTriggerMove(HWND hWnd, const int &p_x)
		{
			PostMessage(hWnd, WM_RIGHT_TRIGGER_MOTION, p_x, 0);
		}

		static void PostLeftTriggerReset(HWND hWnd)
		{
			PostMessage(hWnd, WM_LEFT_TRIGGER_MOTION, 0, 0);
		}

		static void PostRightTriggerReset(HWND hWnd)
		{
			PostMessage(hWnd, WM_RIGHT_TRIGGER_MOTION, 0, 0);
		}

		static void PostDirectionalCrossMotion(HWND hWnd, const int &p_x, const int &p_y)
		{
			LPARAM packedParams = MAKELPARAM(static_cast<WORD>(p_x), static_cast<WORD>(p_y));
			PostMessage(hWnd, WM_DIRECTIONAL_CROSS_MOTION, 0, packedParams);
		}

		static void PostDirectionalCrossReset(HWND hWnd)
		{
			PostMessage(hWnd, WM_DIRECTIONAL_CROSS_MOTION, 0, 0);
		}

	public:
		ControllerInputThread() :
			_worker(L"ControllerInputThread"),
			_directInput(nullptr),
			_controller(nullptr)
		{
		}

		~ControllerInputThread()
		{
			if (_controller)
			{
				_controller->Unacquire();
				_controller->Release();
			}
			if (_directInput)
			{
				_directInput->Release();
			}
		}

		void bind(HWND hWnd)
		{
			_hWnd = hWnd;
		}

		void start()
		{
			_worker.addPreparationStep([this]() { InitializeDirectInput(); }).relinquish();
			_worker.addExecutionStep([this]() { PollController(); }).relinquish();

			_worker.start();
		}

               void stop()
               {
                       _worker.stop();
               }
       };
#else
       class ControllerInputThread
       {
       public:
               void bind(::Window) {}
               void start() {}
               void stop() {}
       };
#endif
}
