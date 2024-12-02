#include "structure/system/device/spk_controller_input_thread.hpp"

namespace spk
{
		bool ControllerInputThread::InitializeDirectInput()
		{
			HRESULT hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&_directInput, NULL);
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

		void ControllerInputThread::PollController()
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
				unsigned short rightTriggerValue = _controllerState.lZ & 0x00FF; // Extract low byte for R2

				unsigned short prevLeftTriggerValue = (_prevControllerState.lZ & 0xFF00) >> 8; // Extract high byte for L2
				unsigned short prevRightTriggerValue = _prevControllerState.lZ & 0x00FF; // Extract low byte for R2

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
					PostRightDirectionalCrossMotion(_hWnd, 0, 0); break;
				case 0:
					PostRightDirectionalCrossMotion(_hWnd, 0, 1); break;
				case 4500:
					PostRightDirectionalCrossMotion(_hWnd, 1, 1); break;
				case 9000:
					PostRightDirectionalCrossMotion(_hWnd, 1, 0); break;
				case 13500:
					PostRightDirectionalCrossMotion(_hWnd, 1, -1); break;
				case 18000:
					PostRightDirectionalCrossMotion(_hWnd, 0, -1); break;
				case 22500:
					PostRightDirectionalCrossMotion(_hWnd, -1, -1); break;
				case 27000:
					PostRightDirectionalCrossMotion(_hWnd, -1, 0); break;
				case 31500:
					PostRightDirectionalCrossMotion(_hWnd, -1, 1); break;
				}
				_prevControllerState.rgdwPOV[0] = _controllerState.rgdwPOV[0];
			}
		}

		void ControllerInputThread::PostButtonPress(HWND hWnd, spk::Controller::Button button)
		{
			PostMessage(hWnd, WM_CONTROLLER_BUTTON_PRESS, static_cast<int>(button), 0);
		}

		void ControllerInputThread::PostButtonRelease(HWND hWnd, spk::Controller::Button button)
		{
			PostMessage(hWnd, WM_CONTROLLER_BUTTON_RELEASE, static_cast<int>(button), 0);
		}

		void ControllerInputThread::PostLeftJoystickMove(HWND hWnd, const unsigned short& p_x, const unsigned short& p_y)
		{
			PostMessage(hWnd, WM_LEFT_JOYSTICK_MOTION, static_cast<WPARAM>(p_x), static_cast<LPARAM>(p_y));
		}

		void ControllerInputThread::PostRightJoystickMove(HWND hWnd, const unsigned short& p_x, const unsigned short& p_y)
		{
			PostMessage(hWnd, WM_RIGHT_JOYSTICK_MOTION, static_cast<WPARAM>(p_x), static_cast<LPARAM>(p_y));
		}

		void ControllerInputThread::PostLeftTriggerMove(HWND hWnd, const int& p_x)
		{
			PostMessage(hWnd, WM_LEFT_TRIGGER_MOTION, p_x, 0);
		}

		void ControllerInputThread::PostRightTriggerMove(HWND hWnd, const int& p_x)
		{
			PostMessage(hWnd, WM_RIGHT_TRIGGER_MOTION, p_x, 0);
		}

		void ControllerInputThread::PostRightDirectionalCrossMotion(HWND hWnd, const int& p_x, const int& p_y)
		{
			LPARAM packedParams = MAKELPARAM(static_cast<WORD>(p_x), static_cast<WORD>(p_y));
			PostMessage(hWnd, WM_DIRECTIONAL_CROSS_MOTION, 0, packedParams);
		}

		ControllerInputThread::ControllerInputThread() :
			_worker(L"ControllerInputThread"),
			_directInput(nullptr),
			_controller(nullptr)
		{

		}

		ControllerInputThread::~ControllerInputThread()
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

		void ControllerInputThread::bind(HWND hWnd)
		{
			_hWnd = hWnd;
		}

		void ControllerInputThread::start()
		{
			_worker.addPreparationStep([this](){
				InitializeDirectInput();
			}).relinquish();
			_worker.addExecutionStep([this](){
					PollController();
				}).relinquish();
			
			_worker.start();
		}

		void ControllerInputThread::stop()
		{
			_worker.stop();
		}
}
