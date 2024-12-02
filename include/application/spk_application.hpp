#pragma once

#include <vector>
#include <map>

#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/thread/spk_persistant_worker.hpp"

#include "structure/spk_safe_pointer.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	/**
	 * @class Application
	 * @brief The core application class responsible for managing threads and execution flow.
	 * 
	 * This class provides functionality to manage multiple workers, add execution and preparation jobs,
	 * and handle the main execution loop. The main thread is always named "MainThread".
	 * 
	 * Example usage:
	 * @code
	 * spk::Application app;
	 * 
	 * auto job = []() { std::cout << "Job executed!" << std::endl; };
	 * app.addExecutionStep(L"MyWorkerThread", job);
	 * 
	 * app.run();
	 * app.quit(0);
	 * @endcode
	 */
	class Application
	{
	public:
		static constexpr const wchar_t* MainThreadName = L"MainThread"; /**< @brief Name of the main thread. */

		using Job = spk::ContractProvider::Job; /**< @brief Alias for job type. */
		using Contract = spk::ContractProvider::Contract; /**< @brief Alias for execution contract type. */
		using PreparationJob = spk::ContractProvider::Job; /**< @brief Alias for preparation job type. */
		using PreparationContract = spk::ContractProvider::Contract; /**< @brief Alias for preparation contract type. */


	private:
		std::atomic<bool> _isRunning;
		std::atomic<int> _errorCode;

		std::unordered_map<std::wstring, std::unique_ptr<spk::PersistantWorker>> _workers;
		spk::SafePointer<spk::PersistantWorker> _mainThreadWorker;

	public:
		/**
		 * @brief Constructs the Application object.
		 * Initializes the main thread worker.
		 */
		Application();

		/**
		 * @brief Destroys the Application object.
		 * Ensures all threads are cleaned up properly.
		 */
		~Application();

		/**
		 * @brief Retrieves or creates a worker for the specified thread name.
		 * 
		 * @param p_threadName The name of the thread.
		 * @return Safe pointer to the worker.
		 */
		spk::SafePointer<spk::PersistantWorker> worker(const std::wstring& p_threadName);

		/**
		 * @brief Adds an execution step to a specified thread.
		 * 
		 * @param p_threadName The name of the thread.
		 * @param p_job The job to execute.
		 * @return Contract for the added execution step.
		 */
		Contract addExecutionStep(const std::wstring& p_threadName, const Job& p_job);

		/**
		 * @brief Adds an execution step to the main thread.
		 * 
		 * @param p_job The job to execute.
		 * @return Contract for the added execution step.
		 */
		Contract addExecutionStep(const Job& p_job);

		/**
		 * @brief Adds a preparation step to a specified thread.
		 * 
		 * @param p_threadName The name of the thread.
		 * @param p_job The preparation job to execute.
		 * @return Contract for the added preparation step.
		 */
		PreparationContract addPreparationStep(const std::wstring& p_threadName, const PreparationJob& p_job);

		/**
		 * @brief Adds a preparation step to the main thread.
		 * 
		 * @param p_job The preparation job to execute.
		 * @return Contract for the added preparation step.
		 */
		PreparationContract addPreparationStep(const PreparationJob& p_job);

		/**
		 * @brief Runs the application.
		 * 
		 * Starts all workers except the main thread and executes jobs until the application is stopped.
		 * @return The application's exit code.
		 */
		int run();

		/**
		 * @brief Stops the application.
		 * 
		 * @param p_errorCode The exit code to return.
		 */
		void quit(int p_errorCode);

		/**
		 * @brief Checks if the application is currently running.
		 * 
		 * @return `true` if running, `false` otherwise.
		 */
		bool isRunning() const;
	};
}