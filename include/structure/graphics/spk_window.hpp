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
	class Window
	{
		friend struct Event;
		friend class GraphicalApplication;
		friend class SystemModule;

	private:
		std::unique_ptr<Widget> _rootWidget;
		
		std::wstring _title;
		spk::Viewport _viewport;
		spk::PersistantWorker _windowRendererThread;
		spk::PersistantWorker _windowUpdaterThread;
		ControllerInputThread _controllerInputThread;

		HWND _hwnd;
		HDC _hdc;
		HGLRC _hglrc;
		std::optional<long long> _pendingUpdateRate;
		UINT_PTR _updateTimerID = 0;

		MouseModule mouseModule;
		KeyboardModule keyboardModule;
		SystemModule systemModule;
		UpdateModule updateModule;
		PaintModule paintModule;
		ControllerModule controllerModule;

		std::map<UINT, spk::IModule*> _subscribedModules;
		std::function<void(spk::SafePointer<spk::Window>)> _onClosureCallback = nullptr;

		void _initialize(const std::function<void(spk::SafePointer<spk::Window>)>& p_onClosureCallback);
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void _createContext();
		void _createOpenGLContext();
		void _destroyOpenGLContext();

		bool _receiveEvent(UINT uMsg, WPARAM wParam, LPARAM lParam);

	public:
		Window(const std::wstring& p_title, const spk::Geometry2D& p_geometry);
		~Window();

		void move(const spk::Geometry2D::Point& p_newPosition);
		void resize(const spk::Geometry2D::Size& p_newSize);
		void close();
		void clear();
		void swap() const;

		void setUpdateRate(const long long& p_durationInMillisecond);
		void clearUpdateRate();

		void pullEvents();
		void bindModule(spk::IModule* p_module);

		void requestPaint() const;
		void requestUpdate() const;

		spk::SafePointer<Widget> widget() const;
		operator spk::SafePointer<Widget>() const;

		const std::wstring& title() const;
		const spk::Geometry2D& geometry() const;
	};
}