#pragma once

#include "application/spk_application.hpp"
#include "widget/spk_widget.hpp"

#include "application/module/spk_update_module.hpp"

namespace spk
{
	class ConsoleApplication : public spk::Application
	{
	private:
		std::unique_ptr<Widget> _centralWidget;
		spk::UpdateModule _updateModule;
		HWND _windowHandle;

		HWND createBackgroundHandle(const std::wstring &p_title);

	public:
		ConsoleApplication(const std::wstring &p_title);

		spk::SafePointer<Widget> centralWidget() const;

		operator spk::SafePointer<Widget>() const;
	};
}
