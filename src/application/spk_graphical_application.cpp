#include "application/spk_graphical_application.hpp"
#include <codecvt>
#include <locale>

#include "utils/spk_string_utils.hpp"

namespace spk
{
	GraphicalApplication::GraphicalApplication()
	{
		WNDCLASSEX wc = {0};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = Window::WindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = "SPKWindowClass";
		wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

		WNDCLASSEX existingClass = {0};
		if (!GetClassInfoEx(GetModuleHandle(nullptr), "SPKWindowClass", &existingClass))
		{
			if (!RegisterClassEx(&wc))
			{
				DWORD error = GetLastError();
				std::cerr << "RegisterClass failed with error: " << error << std::endl;
				throw std::runtime_error("Failed to register window class.");
			}
		}

		this->addExecutionStep(
				[&]()
				{
					while (_windowToRemove.empty() == false)
					{
						closeWindow(_windowToRemove.pop());
						if (_windows.size() == 0)
						{
							quit(0);
						}
					}
				})
			.relinquish();
	}

	spk::SafePointer<Window> GraphicalApplication::createWindow(const std::wstring &p_title, const spk::Geometry2D &p_geometry)
	{
		if (_windows.contains(p_title) == true)
		{
			throw std::runtime_error("Can't create a second window named [" + StringUtils::wstringToString(p_title) + "]");
		}
		_windows[p_title] = std::make_unique<spk::Window>(p_title, p_geometry);

		_windows[p_title]->_initialize([&](spk::SafePointer<spk::Window> windowPtr) { _windowToRemove.push(std::move(windowPtr)); });

		return (_windows[p_title].get());
	}

	void GraphicalApplication::closeWindow(spk::SafePointer<Window> p_windowToClose)
	{
		_windows.erase(p_windowToClose->title());
	}
}