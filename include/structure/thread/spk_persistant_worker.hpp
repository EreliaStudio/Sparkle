#pragma once

#include "structure/thread/spk_thread.hpp"
#include "structure/design_pattern/spk_contract_provider.hpp"

namespace spk
{
	/**
	 * @class PersistantWorker
	 * @brief A thread-based worker for managing and executing jobs persistently.
	 *
	 * The `PersistantWorker` class extends the functionality of `spk::Thread` to manage two types of job queues:
	 * preparation jobs and execution jobs. The worker runs persistently, executing the jobs added to these queues.
	 *
	 * ### Example
	 * @code
	 * spk::PersistantWorker worker(L"MyWorker");
	 * worker.addPreparationStep([]() {
	 *	 // Perform initialization.
	 * });
	 * worker.addExecutionStep([]() {
	 *	 // Main task execution.
	 * });
	 * worker.start();
	 * // Perform other operations...
	 * worker.stop();
	 * worker.join();
	 * @endcode
	 */
	class PersistantWorker : public spk::Thread
	{
	public:
		using Job = spk::ContractProvider::Job; ///< Alias for job type.
		using Contract = spk::ContractProvider::Contract; ///< Alias for contract type.

	private:
		std::atomic<bool> _running; ///< Indicates whether the worker is running.

		spk::ContractProvider _preparationJobs; ///< Provider for preparation jobs.
		spk::ContractProvider _executionJobs; ///< Provider for execution jobs.

	public:
		/**
		 * @brief Constructs a `PersistantWorker` with a specified name.
		 * @param p_name Name of the worker thread.
		 */
		PersistantWorker(const std::wstring& p_name);

		/**
		 * @brief Destructor for the `PersistantWorker`.
		 *
		 * Ensures proper cleanup and stops the worker if it is running.
		 */
		~PersistantWorker();

		/**
		 * @brief Stops the worker, ensuring all jobs are completed.
		 */
		void stop();

		/**
		 * @brief Joins the worker thread, waiting for its completion.
		 */
		void join();

		/**
		 * @brief Adds a job to the preparation step queue.
		 * @param p_job The job to add.
		 * @return A contract associated with the added job.
		 */
		Contract addPreparationStep(const Job& p_job);

		/**
		 * @brief Adds a job to the execution step queue.
		 * @param p_job The job to add.
		 * @return A contract associated with the added job.
		 */
		Contract addExecutionStep(const Job& p_job);

		/**
		 * @brief Accesses the preparation jobs provider.
		 * @return Reference to the preparation jobs provider.
		 */
		spk::ContractProvider& preparationJobs();

		/**
		 * @brief Accesses the execution jobs provider.
		 * @return Reference to the execution jobs provider.
		 */
		spk::ContractProvider& executionJobs();
	};
}
