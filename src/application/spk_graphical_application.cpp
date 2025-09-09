#include "application/spk_graphical_application.hpp"
#include <codecvt>
#include <locale>

#include "utils/spk_string_utils.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	GraphicalApplication::GraphicalApplication()
	{
		WNDCLASSEX windowClass = {0};
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		windowClass.lpfnWndProc = Window::_windowProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = GetModuleHandle(nullptr);
		windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		windowClass.lpszMenuName = nullptr;
		windowClass.lpszClassName = "SPKWindowClass";
		windowClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

		WNDCLASSEX existingClass = {0};
		if (!GetClassInfoEx(GetModuleHandle(nullptr), "SPKWindowClass", &existingClass))
		{
			if (!RegisterClassEx(&windowClass))
			{
				DWORD error = GetLastError();
				std::cerr << "RegisterClass failed with error: " << error << std::endl;
				GENERATE_ERROR("Failed to register window class.");
			}
		}

		this->addExecutionStep(
				[&]()
				{
					while (_windowToRemove.empty() == false)
					{
						closeWindow(_windowToRemove.pop());
						if (_windows.empty())
						{
							quit(0);
						}
					}
				})
			.relinquish();
	}

	GraphicalApplication::~GraphicalApplication()
	{
	}

	spk::SafePointer<Window> GraphicalApplication::createWindow(const std::wstring &p_title, const spk::Geometry2D &p_geometry)
	{
		if (_windows.contains(p_title) == true)
		{
			GENERATE_ERROR("Can't create a second window named [" + StringUtils::wstringToString(p_title) + "]");
		}
		_windows[p_title] = std::make_unique<spk::Window>(p_title, p_geometry);

		_windows[p_title]->_initialize([&](spk::SafePointer<spk::Window> p_windowPtr) { _windowToRemove.push(std::move(p_windowPtr)); });

		return (_windows[p_title].get());
	}

	void GraphicalApplication::closeWindow(spk::SafePointer<Window> p_windowToClose)
	{
		_windows.erase(p_windowToClose->title());
	}
}