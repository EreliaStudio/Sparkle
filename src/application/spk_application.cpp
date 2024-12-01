#include "application/spk_application.hpp"

namespace spk
{
	Application::Application()
	{
		_mainThreadWorker = worker(MainThreadName);
	}

	Application::~Application()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	spk::SafePointer<spk::PersistantWorker> Application::worker(const std::wstring& p_threadName)
	{
		if (_workers.contains(p_threadName) == false)
			_workers[p_threadName] = std::make_unique<spk::PersistantWorker>(p_threadName);

		return (_workers[p_threadName].get());
	}

	Application::Contract Application::addExecutionStep(const std::wstring& p_threadName, const Job& p_job)
	{
		return (worker(p_threadName)->addExecutionStep(p_job));
	}

	Application::Contract Application::Application::addExecutionStep(const Job& p_job)
	{
		return (_workers[MainThreadName]->addExecutionStep(p_job));
	}

	Application::PreparationContract Application::addPreparationStep(const std::wstring& p_threadName, const PreparationJob& p_job)
	{
		return (worker(p_threadName)->addPreparationStep(p_job));
	}

	Application::PreparationContract Application::addPreparationStep(const PreparationJob& p_job)
	{
		return (_workers[MainThreadName]->addPreparationStep(p_job));
	}

	int Application::run()
	{
		_isRunning = true;

		try
		{
			for (auto& [key, worker] : _workers)
			{
				if (key != MainThreadName)
					worker->start();
			}

			spk::cout.setPrefix(L"MainThread");
			_mainThreadWorker->preparationJobs().trigger();

			while (_isRunning == true)
			{
				_mainThreadWorker->executionJobs().trigger();
			}
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}

		for (auto& [key, worker] : _workers)
		{
			if (key != MainThreadName)
				worker->stop();
		}

		return (_errorCode);
	}

	void Application::quit(int p_errorCode)
	{
		_isRunning = false;
		_errorCode = p_errorCode;
	}

	bool Application::isRunning() const
	{
		return (_isRunning);
	}
}