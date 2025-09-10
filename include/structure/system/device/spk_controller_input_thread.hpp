#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>

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
		HWND _hWnd = nullptr;
		bool _initialization = false;
		IDirectInput8 *_directInput = nullptr;
		IDirectInputDevice8 *_controller = nullptr;
		DIJOYSTATE _controllerState = DIJOYSTATE();
		DIJOYSTATE _prevControllerState = DIJOYSTATE();

		bool _initializeDirectInput()
		{
			HRESULT hr = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)&_directInput, nullptr);
			if (FAILED(hr))
			{
				return false;
			}

			hr = _directInput->CreateDevice(GUID_Joystick, &_controller, nullptr);
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

		void _pollController()
		{
			if (_controller == nullptr)
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
				if (((_controllerState.rgbButtons[i] & 0x80) != 0) && ((_prevControllerState.rgbButtons[i] & 0x80) == 0))
				{
					_postButtonPress(_hWnd, static_cast<spk::Controller::Button>(i));
					_prevControllerState.rgbButtons[i] = _controllerState.rgbButtons[i];
				}
				else if (((_controllerState.rgbButtons[i] & 0x80) == 0) && ((_prevControllerState.rgbButtons[i] & 0x80) != 0))
				{
					_postButtonRelease(_hWnd, static_cast<spk::Controller::Button>(i));
					_prevControllerState.rgbButtons[i] = _controllerState.rgbButtons[i];
				}
			}

			if (_controllerState.lX != _prevControllerState.lX || _controllerState.lY != _prevControllerState.lY)
			{
				_postLeftJoystickMove(_hWnd, static_cast<unsigned short>(_controllerState.lX), static_cast<unsigned short>(_controllerState.lY));
				_prevControllerState.lX = _controllerState.lX;
				_prevControllerState.lY = _controllerState.lY;
			}

			if (_controllerState.lRx != _prevControllerState.lRx || _controllerState.lRy != _prevControllerState.lRy)
			{
				_postRightJoystickMove(_hWnd, static_cast<unsigned short>(_controllerState.lRx), static_cast<unsigned short>(_controllerState.lRy));
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
					_postLeftTriggerMove(_hWnd, leftTriggerValue);
				}
				if (rightTriggerValue != prevRightTriggerValue)
				{
					_postRightTriggerMove(_hWnd, rightTriggerValue);
				}

				_prevControllerState.lZ = _controllerState.lZ;
			}

			if (_controllerState.rgdwPOV[0] != _prevControllerState.rgdwPOV[0])
			{
				switch (_controllerState.rgdwPOV[0])
				{
				case static_cast<long unsigned int>(-1):
					_postDirectionalCrossMotion(_hWnd, 0, 0);
					break;
				case 0:
					_postDirectionalCrossMotion(_hWnd, 0, 1);
					break;
				case 4500:
					_postDirectionalCrossMotion(_hWnd, 1, 1);
					break;
				case 9000:
					_postDirectionalCrossMotion(_hWnd, 1, 0);
					break;
				case 13500:
					_postDirectionalCrossMotion(_hWnd, 1, -1);
					break;
				case 18000:
					_postDirectionalCrossMotion(_hWnd, 0, -1);
					break;
				case 22500:
					_postDirectionalCrossMotion(_hWnd, -1, -1);
					break;
				case 27000:
					_postDirectionalCrossMotion(_hWnd, -1, 0);
					break;
				case 31500:
					_postDirectionalCrossMotion(_hWnd, -1, 1);
					break;
				}
				_prevControllerState.rgdwPOV[0] = _controllerState.rgdwPOV[0];
			}
		}

		static void _postButtonPress(HWND p_hWnd, spk::Controller::Button p_button)
		{
			PostMessage(p_hWnd, WM_CONTROLLER_BUTTON_PRESS, static_cast<int>(p_button), 0);
		}

		static void _postButtonRelease(HWND p_hWnd, spk::Controller::Button p_button)
		{
			PostMessage(p_hWnd, WM_CONTROLLER_BUTTON_RELEASE, static_cast<int>(p_button), 0);
		}

		static void _postLeftJoystickMove(HWND p_hWnd, const unsigned short &p_x, const unsigned short &p_y)
		{
			PostMessage(p_hWnd, WM_LEFT_JOYSTICK_MOTION, static_cast<WPARAM>(p_x), static_cast<LPARAM>(p_y));
		}

		static void _postRightJoystickMove(HWND p_hWnd, const unsigned short &p_x, const unsigned short &p_y)
		{
			PostMessage(p_hWnd, WM_RIGHT_JOYSTICK_MOTION, static_cast<WPARAM>(p_x), static_cast<LPARAM>(p_y));
		}

		static void _postLeftJoystickReset(HWND p_hWnd)
		{
			PostMessage(p_hWnd, WM_LEFT_JOYSTICK_RESET, 0, 0);
		}

		static void _postRightJoystickReset(HWND p_hWnd)
		{
			PostMessage(p_hWnd, WM_RIGHT_JOYSTICK_RESET, 0, 0);
		}

		static void _postLeftTriggerMove(HWND p_hWnd, const int &p_x)
		{
			PostMessage(p_hWnd, WM_LEFT_TRIGGER_MOTION, p_x, 0);
		}

		static void _postRightTriggerMove(HWND p_hWnd, const int &p_x)
		{
			PostMessage(p_hWnd, WM_RIGHT_TRIGGER_MOTION, p_x, 0);
		}

		static void _postLeftTriggerReset(HWND p_hWnd)
		{
			PostMessage(p_hWnd, WM_LEFT_TRIGGER_MOTION, 0, 0);
		}

		static void _postRightTriggerReset(HWND p_hWnd)
		{
			PostMessage(p_hWnd, WM_RIGHT_TRIGGER_MOTION, 0, 0);
		}

		static void _postDirectionalCrossMotion(HWND p_hWnd, const int &p_x, const int &p_y)
		{
			LPARAM packedParams = MAKELPARAM(static_cast<WORD>(p_x), static_cast<WORD>(p_y));
			PostMessage(p_hWnd, WM_DIRECTIONAL_CROSS_MOTION, 0, packedParams);
		}

		static void _postDirectionalCrossReset(HWND p_hWnd)
		{
			PostMessage(p_hWnd, WM_DIRECTIONAL_CROSS_MOTION, 0, 0);
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
			if (_controller != nullptr)
			{
				_controller->Unacquire();
				_controller->Release();
			}
			if (_directInput != nullptr)
			{
				_directInput->Release();
			}
		}

		void bind(HWND p_hWnd)
		{
			_hWnd = p_hWnd;
		}

		void start()
		{
			_worker.addPreparationStep([this]() { _initializeDirectInput(); }).relinquish();
			_worker.addExecutionStep([this]() { _pollController(); }).relinquish();

			_worker.start();
		}

		void stop()
		{
			_worker.stop();
		}
	};
}
