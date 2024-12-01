#pragma once

#include "application/spk_application.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	class ConsoleApplication : public spk::Application
	{
	private:
		std::unique_ptr<Widget> _centralWidget;
		HWND _hwnd;

		HWND createBackgroundHandle(const std::wstring& p_title);

	public:
		ConsoleApplication(const std::wstring& p_title);

		spk::SafePointer<Widget> centralWidget() const;

		operator spk::SafePointer<Widget>() const;
	};
}