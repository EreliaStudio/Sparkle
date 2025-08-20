#include "application/spk_console_application.hpp"

namespace spk
{
	HWND ConsoleApplication::createBackgroundHandle(const std::wstring &p_title)
	{
		const std::wstring className = L"DummyWindowClass";

		WNDCLASSW windowClass = {};

		windowClass.lpfnWndProc = DefWindowProc;
		windowClass.hInstance = GetModuleHandle(nullptr);
		windowClass.lpszClassName = className.c_str();

		RegisterClassW(&windowClass);

		HWND windowHandle = CreateWindowExW(
			0,					 // Optional window styles.
			className.c_str(),	 // Window class
			p_title.c_str(),	 // Window text
			WS_OVERLAPPEDWINDOW, // Window style

			// Size and position
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,

			nullptr,				  // Parent window
			nullptr,				  // Menu
			GetModuleHandle(nullptr), // Instance handle
			nullptr					  // Additional application data
		);

		return (windowHandle);
	}

	ConsoleApplication::ConsoleApplication(const std::wstring &p_title) :
		Application(),
		_centralWidget(std::make_unique<Widget>(L"CentralWidget")),
		_updateModule(_centralWidget.get()),
		_windowHandle(createBackgroundHandle(p_title))
	{
		_centralWidget->activate();
		addExecutionStep(
			[&]()
			{
				spk::Event event(nullptr, WM_UPDATE_REQUEST, 0, 0);

				event.updateEvent.time = SystemUtils::getTime();

				_updateModule.receiveEvent(std::move(event));

				_updateModule.treatMessages();
			})
			.relinquish();
	}

	spk::SafePointer<Widget> ConsoleApplication::centralWidget() const
	{
		return (_centralWidget.get());
	}

	ConsoleApplication::operator spk::SafePointer<Widget>() const
	{
		return (centralWidget());
	}
}
