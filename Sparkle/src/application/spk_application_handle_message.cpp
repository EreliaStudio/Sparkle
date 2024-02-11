#include "application/spk_application.hpp"

namespace spk
{
	LRESULT CALLBACK Application::WindowProc(HWND p_hwnd, UINT p_messageID, WPARAM p_firstParam, LPARAM p_secondParam)
	{
		static Application *pThis = NULL;

		switch (p_messageID)
		{
			case WM_NCCREATE:
			{
				CREATESTRUCT *pCreate = (CREATESTRUCT *)p_secondParam;
				pThis = (Application *)pCreate->lpCreateParams;
				SetWindowLongPtr(p_hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
				return (TRUE);
			}
			default:
			{
				if (pThis != NULL)
				{
					return pThis->_handleMessage(p_hwnd, p_messageID, p_firstParam, p_secondParam);
				}
				else
				{
					return DefWindowProc(p_hwnd, p_messageID, p_firstParam, p_secondParam);
				}
			}
		}
		
	}

	LRESULT Application::_handleMessage(const HWND& p_hwnd, const UINT& p_messageID, const WPARAM& p_firstParam, const LPARAM& p_secondParam)
	{
		switch (p_messageID)
		{
			case WM_QUIT:
			case WM_DESTROY:
			{
				quit(0);
				break;
			}
			case WM_SIZE:
			{
				unsigned int width = LOWORD(p_secondParam);
				unsigned int height = HIWORD(p_secondParam);

				_handle.resize(Vector2Int(width, height));
				if (_creationComplete == true)
				{
					_centralWidget->resize(Vector2Int(0, 0), Vector2UInt(width, height));
				}
				_updaterJobs.push_back([&, width, height](){});
				break;
			}
			case WM_MOVE:
			{
				unsigned int x = LOWORD(p_secondParam);
				unsigned int y = HIWORD(p_secondParam);

				_handle.place(Vector2Int(x, y));

				_updaterJobs.push_back([&, x, y](){});
				break;
			}

			case WM_LBUTTONDOWN:
			{
				_updaterJobs.push_back([&](){
						_mouse.pressButton(Mouse::Left);
						_inputDecoder.add(Input{Mouse::Left, InputState::Pressed});
					});
				break;
			}
			case WM_MBUTTONDOWN:
			{
				_updaterJobs.push_back([&](){
						_mouse.pressButton(Mouse::Middle);
						_inputDecoder.add(Input{Mouse::Middle, InputState::Pressed});
					});
				break;
			}
			case WM_RBUTTONDOWN:
			{
				_updaterJobs.push_back([&](){
					_mouse.pressButton(Mouse::Right);
					_inputDecoder.add(Input{Mouse::Right, InputState::Pressed});
				});
				break;
			}
			case WM_LBUTTONUP:
			{
				_updaterJobs.push_back([&](){
					_mouse.releaseButton(Mouse::Left);
					_inputDecoder.add(Input{Mouse::Left, InputState::Released});
				});
				break;
			}
			case WM_MBUTTONUP:
			{
				_updaterJobs.push_back([&](){
					_mouse.releaseButton(Mouse::Middle);
					_inputDecoder.add(Input{Mouse::Middle, InputState::Released});
				});
				break;
			}
			case WM_RBUTTONUP:
			{
				_updaterJobs.push_back([&](){
					_mouse.releaseButton(Mouse::Right);
					_inputDecoder.add(Input{Mouse::Right, InputState::Released});
				});
				break;
			}
			case WM_XBUTTONDOWN :
			{
				switch (GET_XBUTTON_WPARAM (p_firstParam))
				{
					case (MK_XBUTTON1):
					{
						_updaterJobs.push_back([&](){
							_mouse.pressButton(Mouse::Button3);
							_inputDecoder.add(Input{Mouse::Button3, InputState::Pressed});
						});
						break;
					}
					case (MK_XBUTTON2):
					{
						_updaterJobs.push_back([&](){
							_mouse.pressButton(Mouse::Button4);
							_inputDecoder.add(Input{Mouse::Button4, InputState::Pressed});
						});
						break;
					}
					default:
					{
						throwException("Unknow release mouse button event pulled");
					}
				}
				break;
			}
			case WM_XBUTTONUP :
			{
				switch (GET_XBUTTON_WPARAM (p_firstParam))
				{
					case (MK_XBUTTON1):
					{
						_updaterJobs.push_back([&](){
							_mouse.releaseButton(Mouse::Button3);
							_inputDecoder.add(Input{Mouse::Button3, InputState::Released});
						});
						break;
					}
					case (MK_XBUTTON2):
					{
						_updaterJobs.push_back([&](){
							_mouse.releaseButton(Mouse::Button4);
							_inputDecoder.add(Input{Mouse::Button4, InputState::Released});
						});
						break;
					}
					default:
					{
						throwException("Unknow release mouse button event pulled");
					}
				}
				break;
			}
			case WM_MOUSEHWHEEL:
			{
				spk::Vector2Int wheelPosition = Vector2Int(0, GET_WHEEL_DELTA_WPARAM(p_firstParam));

				_updaterJobs.push_back([&, wheelPosition](){
					_mouse.editWheelPosition(wheelPosition);
				});
				break;
			}
			case WM_MOUSEWHEEL:
			{
				spk::Vector2Int wheelPosition = Vector2Int(GET_WHEEL_DELTA_WPARAM(p_firstParam), 0);

				_updaterJobs.push_back([&, wheelPosition](){
					_mouse.editWheelPosition(wheelPosition);				
				});
				break;
			}
			case WM_MOUSEMOVE:
			{
				spk::Vector2Int mousePosition = Vector2Int(LOWORD(p_secondParam), HIWORD(p_secondParam));

				_updaterJobs.push_back([&, mousePosition](){
					_mouse.setMousePosition(mousePosition);
				});
				break;
			}

			case WM_CHAR:
			{
				unsigned int value = static_cast<unsigned int>(p_firstParam);

				_updaterJobs.push_back([&](){
					
				});
				break;
			}
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
			{
				unsigned int value = static_cast<unsigned int>(p_firstParam);

				_updaterJobs.push_back([&, value](){
					_keyboard.pressKey(value);
					_inputDecoder.add(Input{static_cast<Keyboard::Key>(value), InputState::Pressed});
				});
				break;
			}
			case WM_SYSKEYUP:
			case WM_KEYUP:
			{
				unsigned int value = static_cast<unsigned int>(p_firstParam);

				_updaterJobs.push_back([&, value](){
					_keyboard.releaseKey(value);
					_inputDecoder.add(Input{static_cast<Keyboard::Key>(value), InputState::Released});
				});
				break;
			}

			default:
			{
				return DefWindowProc(p_hwnd, p_messageID, p_firstParam, p_secondParam);
			}
		}
		return (TRUE);
	}
}