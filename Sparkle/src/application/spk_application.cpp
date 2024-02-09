#include "application/spk_application.hpp"

#include <thread>
#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{
	void Application::_pullMessage()
	{
		MSG msg = {};

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			int translateResult = TranslateMessage(&msg);
			int dispatchResult = static_cast<int>(DispatchMessage(&msg));
		}
	}

	void Application::_executeRendererTick()
	{
		_pullMessage();

		_handle.clear();

		_centralWidget.render();

		_handle.swap();
	}

	void Application::_executeUpdaterTick()
	{
		_timeMetrics.update();

		while (_updaterJobs.empty() == false)
		{
			_updaterJobs.pop_front()();
		}
	
		_centralWidget.update();

		_keyboard.update();
		_mouse.update();

	}

	void Application::_executeRendererLoop()
	{
		while (_isRunning == true)
		{
			_fpsCounter.trigger(10);

			_executeRendererTick();

			if (_counterTimer.isTimedOut() == true)
			{
				_fpsCounter.save();
				_upsCounter.save();
				_counterTimer.start();
			}
		}
	}

	void Application::_executeUpdaterLoop()
	{
		while (_isRunning == true)
		{
			_upsCounter.trigger(10);

			_executeUpdaterTick();
		}
	}

	int Application::_runMultithread()
	{
		std::thread updaterThread([&](){
			setAsActiveApplication();
			_executeUpdaterLoop();
		});

		_executeRendererLoop();

		if (updaterThread.joinable() == true)
			updaterThread.join();

		return (0);
	}

	int Application::_runMonothread()
	{
		while (_isRunning == true)
		{
			_fpsCounter.trigger(10);
			_upsCounter.trigger(10);

			_executeRendererTick();
			_executeUpdaterTick();

			if (_counterTimer.isTimedOut() == true)
			{
				_fpsCounter.save();
				_upsCounter.save();
				_counterTimer.start();
			}
		}
		return (0);
	}

	Application::Application(const std::string& p_title, const Vector2UInt& p_size, const Mode& p_mode) :
		_mode(p_mode),
		_updaterJobs(),
		_handle(this, p_title, p_size),
		_counterTimer(100),
		_profiler(),
		_fpsCounter(_profiler.metric<TriggerMetric>("FPSCounter")),
		_upsCounter(_profiler.metric<TriggerMetric>("UPSCounter"))
	{
		if (_activeApplication == nullptr)
			setAsActiveApplication();
		_centralWidget.setGeometry(spk::Vector2Int(0, 0), size());
		Viewport::_mainViewport = &(_centralWidget.viewport());
		_creationComplete = true;
	}

	Application::~Application()
	{
		std::cout << _profiler.emitReport() << std::endl;
	}

	int Application::run()
	{
		_isRunning = true;
		_counterTimer.start();
		if (_mode == Mode::Multithread)
			return (_runMultithread());
		else
			return (_runMonothread());
	}

	void Application::quit(int p_errorCode)
	{
		_errorCode = p_errorCode;
		_isRunning = false;
	}

	const spk::Vector2UInt& Application::size() const
	{
		return (_handle.size());
	}

	const Keyboard& Application::keyboard() const
	{
		return (_keyboard);
	}

	const Mouse& Application::mouse() const
	{
		return (_mouse);
	}

	const TimeMetrics& Application::timeMetrics() const
	{
		return (_timeMetrics);
	}

	Profiler& Application::profiler()
	{
		return (_profiler);
	}
}