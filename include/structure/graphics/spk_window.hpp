#pragma once

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
#include <set>
#include <unordered_set>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <Windows.h>

#include "application/module/spk_controller_module.hpp"
#include "application/module/spk_keyboard_module.hpp"
#include "application/module/spk_mouse_module.hpp"
#include "application/module/spk_paint_module.hpp"
#include "application/module/spk_system_module.hpp"
#include "application/module/spk_timer_module.hpp"
#include "application/module/spk_update_module.hpp"

namespace spk
{
	class Window
	{
		friend struct Event;
		friend class GraphicalApplication;
		friend class SystemModule;
		friend class ControllerModule;
		friend class KeyboardModule;
		friend class MouseModule;
		friend class UpdateModule;
		friend class PaintModule;

	private:
		std::unique_ptr<Widget> _rootWidget;

		std::wstring _title;
		spk::PersistantWorker _windowRendererThread;
		spk::PersistantWorker _windowUpdaterThread;
		ControllerInputThread _controllerInputThread;

		HWND _hwnd;
		HDC _hdc;
		HGLRC _hglrc;

		std::unordered_map<std::wstring, HCURSOR> _cursors;
		HCURSOR _currentCursor;
		HCURSOR _savedCursor;

		mutable bool _isPaintRequestAllowed = true;

		std::recursive_mutex _timerMutex;
		std::set<UINT_PTR> _timers;
		std::deque<std::pair<int, long long>> _pendingTimerCreations;
		std::deque<int> _pendingTimerDeletions;

		MouseModule _mouseModule;
		KeyboardModule _keyboardModule;
		SystemModule _systemModule;
		UpdateModule _updateModule;
		PaintModule _paintModule;
		ControllerModule _controllerModule;
		TimerModule _timerModule;

		std::map<UINT, spk::IModule *> _subscribedModules;
		std::function<void(spk::SafePointer<spk::Window>)> _onClosureCallback = nullptr;

		void _initialize(const std::function<void(spk::SafePointer<spk::Window>)> &p_onClosureCallback);
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void _createContext();
		void _createOpenGLContext();
		void _destroyOpenGLContext();

		bool _receiveEvent(UINT uMsg, WPARAM wParam, LPARAM lParam);

		void _handlePendingTimer();
		UINT_PTR _createTimer(int p_id, const long long &p_durationInMillisecond);
		void _deleteTimer(UINT_PTR p_id);
		void _removeAllTimers();

	public:
		Window(const std::wstring &p_title, const spk::Geometry2D &p_geometry);
		~Window();

		void move(const spk::Geometry2D::Point &p_newPosition);
		void resize(const spk::Geometry2D::Size &p_newSize);
		void close();
		void clear();
		void swap() const;

		void addCursor(const std::wstring &p_cursorName, const std::filesystem::path &p_cursorPath);
		void setCursor(const std::wstring &p_cursorName);

		void setUpdateTimer(const long long &p_durationInMillisecond);
		void removeUpdateTimer();
		void addTimer(int p_id, const long long &p_durationInMillisecond);
		void removeTimer(int p_id);

		void pullEvents();
		void bindModule(spk::IModule *p_module);

		void allowPaintRequest();
		void requestPaint() const;
		void allowUpdateRequest();
		void requestUpdate() const;
		void requestResize(const spk::Vector2Int &p_size) const;

		spk::SafePointer<Widget> widget() const;
		operator spk::SafePointer<Widget>() const;

		const std::wstring &title() const;
		const spk::Geometry2D &geometry() const;
	};
}