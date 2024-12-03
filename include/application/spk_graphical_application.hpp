#pragma once

#include "application/spk_application.hpp"

#include "structure/spk_safe_pointer.hpp"
#include "structure/graphics/spk_window.hpp"

namespace spk
{
	/**
	 * @class GraphicalApplication
	 * @brief A specialized application class for managing graphical windows.
	 * 
	 * This class extends `spk::Application` and provides functionality to create and manage graphical windows
	 * in a graphical application. It supports creating new windows, closing them, and handling the removal of closed windows.
	 * 
	 * Example usage:
	 * @code
	 * spk::GraphicalApplication app;
	 * 
	 * spk::Geometry2D geometry(800, 600);
	 * auto window = app.createWindow(L"My Window", geometry);
	 * 
	 * app.run();
	 * app.quit(0);
	 * @endcode
	 */
	class GraphicalApplication : public Application
	{
	private:
		spk::ThreadSafeQueue<spk::SafePointer<Window>> _windowToRemove;
		std::map<std::wstring, std::unique_ptr<spk::Window>> _windows;

	public:
		/**
		 * @brief Constructs a `GraphicalApplication` instance.
		 * 
		 * Initializes the window class and prepares the system to manage graphical windows.
		 */
		GraphicalApplication();

		/**
		 * @brief Creates a new window.
		 * 
		 * This method creates a window with the specified title and geometry, and initializes it for use.
		 * 
		 * @param p_title The title of the window.
		 * @param p_geometry The geometry (width and height) of the window.
		 * @return A safe pointer to the newly created window.
		 * @throws std::runtime_error if a window with the same title already exists.
		 */
		spk::SafePointer<Window> createWindow(const std::wstring& p_title, const spk::Geometry2D& p_geometry);

		/**
		 * @brief Closes the specified window.
		 * 
		 * This method removes the window from the application and ensures it is properly cleaned up.
		 * 
		 * @param p_windowToClose The window to be closed.
		 */
		void closeWindow(spk::SafePointer<Window> p_windowToClose);
	};
}