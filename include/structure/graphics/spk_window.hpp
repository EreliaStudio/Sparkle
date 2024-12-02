#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "structure/spk_safe_pointer.hpp"
#include "structure/container/spk_thread_safe_queue.hpp"
#include "structure/system/device/spk_controller_input_thread.hpp"
#include "structure/design_pattern/spk_contract_provider.hpp"
#include "widget/spk_widget.hpp"
#include "structure/thread/spk_persistant_worker.hpp"
#include "application/module/spk_module.hpp"
#include "structure/graphics/spk_viewport.hpp"
#include <deque>
#include <map>
#include <unordered_set>
#include <Windows.h>
#include "application/module/spk_mouse_module.hpp"
#include "application/module/spk_keyboard_module.hpp"
#include "application/module/spk_system_module.hpp"
#include "application/module/spk_update_module.hpp"
#include "application/module/spk_paint_module.hpp"
#include "application/module/spk_controller_module.hpp"

namespace spk
{
	/**
	 * @class Window
	 * @brief Represents a graphical window with OpenGL support.
	 * 
	 * The `Window` class manages a rendering context, input handling, and graphical updates.
	 * It integrates with various modules for handling input, updates, and rendering.
	 * 
	 * Example usage:
	 * @code
	 * spk::Geometry2D geometry({100, 100}, {800, 600});
	 * spk::Window window(L"My Window", geometry);
	 * window.setUpdateRate(16); // Set update rate to ~60 FPS
	 * @endcode
	 */
	class Window
	{
		friend struct Event;
		friend class GraphicalApplication;
		friend class SystemModule;

	private:
		std::unique_ptr<Widget> _rootWidget; ///< Root widget of the window.

		std::wstring _title; ///< Window title.
		spk::Viewport _viewport; ///< Viewport defining the rendering area.
		spk::PersistantWorker _windowRendererThread; ///< Thread for rendering.
		spk::PersistantWorker _windowUpdaterThread; ///< Thread for updates.
		ControllerInputThread _controllerInputThread; ///< Thread for controller input.

		HWND _hwnd; ///< Handle to the window.
		HDC _hdc; ///< Handle to the device context.
		HGLRC _hglrc; ///< Handle to the OpenGL rendering context.
		std::optional<long long> _pendingUpdateRate; ///< Pending update rate in milliseconds.
		UINT_PTR _updateTimerID = 0; ///< Timer ID for updates.

		MouseModule mouseModule; ///< Mouse input module.
		KeyboardModule keyboardModule; ///< Keyboard input module.
		SystemModule systemModule; ///< System event module.
		UpdateModule updateModule; ///< Update handling module.
		PaintModule paintModule; ///< Paint handling module.
		ControllerModule controllerModule; ///< Controller input module.

		std::map<UINT, spk::IModule*> _subscribedModules; ///< Subscribed modules for handling events.
		std::function<void(spk::SafePointer<spk::Window>)> _onClosureCallback = nullptr; ///< Callback for window closure.

		void _initialize(const std::function<void(spk::SafePointer<spk::Window>)>& p_onClosureCallback);
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void _createContext();
		void _createOpenGLContext();
		void _destroyOpenGLContext();

		bool _receiveEvent(UINT uMsg, WPARAM wParam, LPARAM lParam);

	public:
		/**
		 * @brief Constructs a window with a title and geometry.
		 * @param p_title Title of the window.
		 * @param p_geometry Geometry defining the position and size of the window.
		 */
		Window(const std::wstring& p_title, const spk::Geometry2D& p_geometry);

		/**
		 * @brief Destructor. Cleans up resources and threads.
		 */
		~Window();

		/**
		 * @brief Moves the window to a new position.
		 * @param p_newPosition The new position of the window.
		 */
		void move(const spk::Geometry2D::Point& p_newPosition);

		/**
		 * @brief Resizes the window.
		 * @param p_newSize The new size of the window.
		 */
		void resize(const spk::Geometry2D::Size& p_newSize);

		/**
		 * @brief Closes the window and stops associated threads.
		 */
		void close();

		/**
		 * @brief Clears the window's content.
		 */
		void clear();

		/**
		 * @brief Swaps the front and back buffers for rendering.
		 */
		void swap() const;

		/**
		 * @brief Sets the update rate for the window.
		 * @param p_durationInMillisecond Update interval in milliseconds.
		 * @throws std::runtime_error If the timer fails to set.
		 */
		void setUpdateRate(const long long& p_durationInMillisecond);

		/**
		 * @brief Clears the update timer.
		 * @throws std::runtime_error If no active or pending timer exists.
		 */
		void clearUpdateRate();

		/**
		 * @brief Processes input and system events.
		 */
		void pullEvents();

		/**
		 * @brief Binds a module to the window for event handling.
		 * @param p_module The module to bind.
		 */
		void bindModule(spk::IModule* p_module);

		/**
		 * @brief Requests a paint event.
		 */
		void requestPaint() const;

		/**
		 * @brief Requests an update event.
		 */
		void requestUpdate() const;

		/**
		 * @brief Retrieves the root widget of the window.
		 * @return A safe pointer to the root widget.
		 */
		spk::SafePointer<Widget> widget() const;

		/**
		 * @brief Implicitly converts the window to its root widget.
		 * @return A safe pointer to the root widget.
		 */
		operator spk::SafePointer<Widget>() const;

		/**
		 * @brief Retrieves the title of the window.
		 * @return A constant reference to the window title.
		 */
		const std::wstring& title() const;

		/**
		 * @brief Retrieves the geometry of the window.
		 * @return A constant reference to the window geometry.
		 */
		const spk::Geometry2D& geometry() const;
	};
}
