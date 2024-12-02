#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <algorithm>
#include <mutex>

namespace spk
{
	/**
	 * @class ContractProvider
	 * @brief Manages subscriptions to jobs and allows triggering them collectively.
	 * 
	 * The `ContractProvider` class is used to manage a collection of subscribed jobs (callbacks). 
	 * Subscribers receive a `Contract` object to manage their subscription, allowing them to 
	 * relinquish or resign the contract when needed.
	 * 
	 * Example usage:
	 * @code
	 * spk::ContractProvider provider;
	 * auto contract = provider.subscribe([]() { std::cout << "Triggered!" << std::endl; });
	 * provider.trigger(); // Outputs: Triggered!
	 * contract.resign(); // Unsubscribes
	 * @endcode
	 */
	class ContractProvider
	{
	public:
		/**
		 * @class Contract
		 * @brief Represents a subscription to a job in the `ContractProvider`.
		 * 
		 * A `Contract` manages the lifecycle of a subscription. It can be resigned to unsubscribe
		 * or relinquished to transfer ownership back to the provider.
		 */
		class Contract
		{
			friend class ContractProvider;

		public:
			using Job = std::function<void()>; /**< @brief Type alias for a job (callback function). */

		private:
			std::shared_ptr<Job> _job = nullptr;
			ContractProvider* _originator = nullptr;

			/**
			 * @brief Constructs a contract for a specific provider and job.
			 * @param p_originator Pointer to the originating provider.
			 * @param p_job Shared pointer to the job.
			 */
			Contract(ContractProvider* p_originator, const std::shared_ptr<Job>& p_job) :
				_originator(p_originator),
				_job(p_job)
			{
			}

		public:
			Contract(const Contract& p_other) = delete; /**< @brief Copy constructor is deleted. */
			Contract& operator=(const Contract& p_other) = delete; /**< @brief Copy assignment is deleted. */

			Contract(Contract&& p_other) = default; /**< @brief Move constructor. */
			Contract& operator=(Contract&& p_other) = default; /**< @brief Move assignment. */

			/**
			 * @brief Destructor. Automatically resigns the contract if valid.
			 */
			~Contract()
			{
				if (isValid())
				{
					resign();
				}
			}

			/**
			 * @brief Checks if the contract is still valid.
			 * @return `true` if the contract is valid, otherwise `false`.
			 */
			bool isValid()
			{
				return (!(_job == nullptr || *_job == nullptr));
			}

			/**
			 * @brief Resigns the contract, unsubscribing the associated job.
			 * @throws std::runtime_error If the contract is already resigned.
			 */
			void resign()
			{
				if (!isValid())
					throw std::runtime_error("Can't resign an already resigned contract");

				*_job = nullptr;
				_originator->unsubscribe(*this);
				_job = nullptr;
			}

			/**
			 * @brief Relinquishes the contract, transferring ownership back to the provider.
			 * @throws std::runtime_error If the contract is already resigned.
			 */
			void relinquish()
			{
				if (!isValid())
					throw std::runtime_error("Can't relinquish an already resigned contract");

				_originator->relinquish(std::move(*this));
				_job = nullptr;
				_originator = nullptr;
			}
		};

		using Job = Contract::Job; /**< @brief Type alias for a job (callback function). */

	private:
		std::vector<std::shared_ptr<Job>> _subscribedJobs;
		std::vector<Contract> _relinquishedContracts;
		std::recursive_mutex _mutex;

	public:
		/**
		 * @brief Constructs a `ContractProvider`.
		 */
		ContractProvider()
		{
		}

		/**
		 * @brief Destructor. Invalidates all contracts upon destruction.
		 */
		~ContractProvider()
		{
			invalidateContracts();
		}

		/**
		 * @brief Invalidates all active contracts.
		 * 
		 * This clears the subscribed jobs and ensures that all jobs are unsubscribed.
		 */
		void invalidateContracts()
		{
			for (auto& job : _subscribedJobs)
			{
				*job = nullptr;
			}
			_subscribedJobs.clear();
		}

		/**
		 * @brief Subscribes a job to the provider.
		 * 
		 * @param p_job The job (callback function) to subscribe.
		 * @return A `Contract` object that manages the subscription.
		 */
		Contract subscribe(const Job& p_job)
		{
			std::shared_ptr<Job> toAdd = std::make_shared<Job>(p_job);

			std::lock_guard<std::recursive_mutex> lock(_mutex);
			_subscribedJobs.push_back(toAdd);

			return Contract(this, toAdd);
		}

		/**
		 * @brief Relinquishes a contract, transferring ownership back to the provider.
		 * 
		 * @param p_contract The contract to relinquish.
		 */
		void relinquish(Contract&& p_contract)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			_relinquishedContracts.push_back(std::move(p_contract));
		}

		/**
		 * @brief Unsubscribes a contract from the provider.
		 * 
		 * @param p_contract The contract to unsubscribe.
		 */
		void unsubscribe(const Contract& p_contract)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			auto it = std::remove(_subscribedJobs.begin(), _subscribedJobs.end(), p_contract._job);
			if (it != _subscribedJobs.end())
			{
				std::shared_ptr<Job> tmp = *it;
				_subscribedJobs.erase(it, _subscribedJobs.end());
				if (tmp != nullptr)
					*tmp = nullptr;
			}
		}

		/**
		 * @brief Triggers all subscribed jobs.
		 * 
		 * Calls each subscribed job in the order they were added.
		 */
		void trigger()
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			for (auto& job : _subscribedJobs)
			{
				if (job != nullptr)
					(*job)();
			}
		}
	};
}
