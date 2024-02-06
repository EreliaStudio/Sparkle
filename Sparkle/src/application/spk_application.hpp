#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <gl/GL.h>
#include <string>
#include <atomic>
#include <functional>
#include <mutex>
#include <deque>

#include "math/spk_matrix4x4.hpp"

#include "input/spk_keyboard.hpp"
#include "input/spk_mouse.hpp"
#include "system/spk_timer.hpp"
#include "system/spk_time_metrics.hpp"
#include "profiler/spk_profiler.hpp"
#include "profiler/spk_trigger_metric.hpp"
#include "data_structure/spk_thread_safe_queue.hpp"
#include "widget/spk_widget.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{
	/**
	 * @class Application
	 * @brief Manages a graphical application lifecycle and central functionalities such as time, mouse or keyboard handling.
	 *
	 * The Application class serves as the core of the Sparkle library, orchestrating the application lifecycle,
	 * managing the main event loop, and facilitating graphics rendering and event handling. It supports both
	 * monothreaded and multithreaded modes of operation for flexibility in application development.
	 *
	 * Features include:
	 * - Window creation and management using OpenGL for rendering.
	 * - Event handling for keyboard and mouse input.
	 * - Time metrics collection.
	 * - Profiling for performance analysis.
	 *
	 * The Application class is designed to be the starting point for applications using the Sparkle library,
	 * allowing developers to easily create, manage, and render graphics content while handling user input
	 * and other application events via the Sparkle IWidget system.
	 *
	 * Usage example:
	 * @code
	 * spk::Application app("Example App", spk::Vector2UInt(800, 600), spk::Application::Mode::Monothread);
	 * 
	 * return (app.run());
	 * @endcode
	 *
	 * @see IWidget, Pipeline, Keyboard, Mouse, TimeMetrics, Profiler
	 */
	class Application
	{
	private:
		static inline Application* _activeApplication = nullptr;
	public:
		static Application* activeApplication()
		{
			return (_activeApplication);
		}

		void setAsActiveApplication()
		{
			_activeApplication = this;
		}

		enum class Mode
		{
			Monothread,
			Multithread
		};

		class CentralWidget : public IWidget
		{
			friend class Application;

			static inline std::string _prerenderingPipelineCode = R"(#version 450
			
			#include <timeConstants>
			#include <screenConstants>
			
			void geometryPass() {}
			void renderPass() {})";
			static inline spk::Pipeline _prerenderingPipeline = spk::Pipeline(_prerenderingPipelineCode);
			spk::Pipeline::Constant& _timeConstant;
			spk::Pipeline::Constant::Element& _timeConstantEpochElement;
			long long oldTime = 0;

		private:
			void _onGeometryChange()
			{
				
			}

			void _onRender()
			{
				
			}

			void _onUpdate()
			{
				long long time = spk::getTime();
				if (time != oldTime)
				{
					oldTime = time;
					_timeConstantEpochElement = static_cast<int>(time % 100000);
					_timeConstant.update();
				}
			}	

			CentralWidget() :
				IWidget("CentralWidget"),
				_timeConstant(_prerenderingPipeline.constant("timeConstants")),
				_timeConstantEpochElement(_timeConstant["epoch"])
			{
				defaultParent = this;
			}

		public:

			~CentralWidget()
			{
				defaultParent = nullptr;
			}

		};

	private:
		static LRESULT CALLBACK WindowProc(HWND p_hwnd, UINT p_messageID, WPARAM p_firstParam, LPARAM p_secondParam);

		class Handle
		{
		private:
			HWND _windowHandle;
			WNDCLASS _frameClass;
			HINSTANCE _applicationInstance;

			HGLRC _openglContext;
			HDC _deviceContext;
			PIXELFORMATDESCRIPTOR _pixelFormatDescriptor;

			Vector2Int _anchor;
			Vector2UInt _size;

		public:
			Handle(Application *p_application, const std::string& p_title, const Vector2UInt& p_size);
			~Handle();

			void configure(Application* p_application);

			void clear();
			void swap();
			void resize(const Vector2UInt& p_newSize);
			void place(const Vector2Int& p_anchor);

			const spk::Vector2UInt& size() const;
		};

		bool _creationComplete = false;
		Mode _mode;
		std::atomic_bool _isRunning;
		std::atomic_int _errorCode;

		spk::ThreadSafeDeque<std::function<void()>> _updaterJobs;

		Keyboard _keyboard;
		Mouse _mouse;

		Profiler _profiler;
		TimeMetrics _timeMetrics;
		Timer _counterTimer;
		TriggerMetric& _fpsCounter;
		TriggerMetric& _upsCounter;

		Handle _handle;
		CentralWidget _centralWidget;

		LRESULT _handleMessage(const HWND& p_hwnd, const UINT& p_messageID, const WPARAM& p_firstParam, const LPARAM& p_secondParam);
		void _pullMessage();

		void _executeRendererTick();
		void _executeUpdaterTick();

		void _executeRendererLoop();
		void _executeUpdaterLoop();

		int _runMultithread();
		int _runMonothread();

	public:
		Application(const std::string& p_title, const Vector2UInt& p_size, const Mode& p_mode = Mode::Monothread);
		~Application();

		int run();
		void quit(int p_error);

		void addRootWidget(IWidget* p_rootWidget)
		{
			_centralWidget.addChild(p_rootWidget);
		}

		const spk::Vector2UInt& size() const;

		const Keyboard& keyboard() const;
		const Mouse& mouse() const;
		
		const TimeMetrics& timeMetrics() const;
		
		Profiler& profiler();
	};
}