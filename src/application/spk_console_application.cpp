#include "application/spk_console_application.hpp"

namespace spk
{
	HWND ConsoleApplication::createBackgroundHandle(const std::wstring& p_title)
	{
		const wchar_t CLASS_NAME[] = L"DummyWindowClass";

		WNDCLASSW wc = { };

		wc.lpfnWndProc = DefWindowProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.lpszClassName = CLASS_NAME;

		RegisterClassW(&wc);

		HWND hwnd = CreateWindowExW(
			0,                              // Optional window styles.
			CLASS_NAME,                     // Window class
			p_title.c_str(),                  // Window text
			WS_OVERLAPPEDWINDOW,            // Window style

			// Size and position
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

			NULL,       // Parent window    
			NULL,       // Menu
			GetModuleHandle(NULL), // Instance handle
			NULL        // Additional application data
		);

		return hwnd;
	}

	ConsoleApplication::ConsoleApplication(const std::wstring& p_title) :
		Application(),
		_centralWidget(std::make_unique<Widget>(L"CentralWidget")),
		_hwnd(createBackgroundHandle(p_title))
	{
		_centralWidget->activate();
		addExecutionStep([&]()
		{
			spk::UpdateEvent event = spk::UpdateEvent();
			centralWidget()->onUpdateEvent(event);
		}).relinquish();
	}

	spk::SafePointer<Widget> ConsoleApplication::centralWidget() const
	{
		return (_centralWidget.get());
	}

	ConsoleApplication::operator spk::SafePointer<Widget>() const
	{
		return centralWidget();
	}
}