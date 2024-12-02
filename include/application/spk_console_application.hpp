#pragma once

#include "application/spk_application.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	/**
	 * @class ConsoleApplication
	 * @brief A specialized application class that includes a central widget and manages a background window handle.
	 * 
	 * This class inherits from `spk::Application` and provides additional functionality to create a central widget
	 * and handle GUI-related tasks through a background window.
	 * 
	 * Example usage:
	 * @code
	 * spk::ConsoleApplication app(L"My Console Application");
	 * 
	 * auto widget = app.centralWidget();
	 * widget->setTitle(L"My Widget");
	 * 
	 * app.run();
	 * app.quit(0);
	 * @endcode
	 */
	class ConsoleApplication : public spk::Application
	{
	private:
		std::unique_ptr<Widget> _centralWidget;
		HWND _hwnd;

		/**
		 * @brief Creates a background window handle.
		 * 
		 * Registers a dummy window class and creates a window handle for the application.
		 * @param p_title The title of the window.
		 * @return The created window handle.
		 */
		HWND createBackgroundHandle(const std::wstring& p_title);

	public:
		/**
		 * @brief Constructs a `ConsoleApplication` instance.
		 * 
		 * Initializes the central widget and creates a background window handle.
		 * @param p_title The title of the console application.
		 */
		ConsoleApplication(const std::wstring& p_title);

		/**
		 * @brief Retrieves the central widget of the application.
		 * 
		 * @return A safe pointer to the central widget.
		 */
		spk::SafePointer<Widget> centralWidget() const;

		/**
		 * @brief Converts the application to a safe pointer to its central widget.
		 * 
		 * This operator allows implicit conversion of the `ConsoleApplication` instance to a `SafePointer<Widget>`.
		 * @return A safe pointer to the central widget.
		 */
		operator spk::SafePointer<Widget>() const;
	};
}