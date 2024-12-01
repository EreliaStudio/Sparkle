#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <algorithm>
#include <mutex>

namespace spk
{
	class ContractProvider
	{
	public:
		class Contract
		{
			friend class ContractProvider;
		public:
			using Job = std::function<void()>;

		private:
			std::shared_ptr<Job> _job = nullptr;
			ContractProvider* _originator = nullptr;

			Contract(ContractProvider* p_originator, const std::shared_ptr<Job>& p_job) :
				_originator(p_originator),
				_job(p_job)
			{

			}

		public:
			Contract(const Contract& p_other) = delete;
			Contract& operator =(const Contract& p_other) = delete;

			Contract(Contract&& p_other) = default;
			Contract& operator =(Contract&& p_other) = default;
		
			~Contract()
			{
				if (isValid() == true)
				{
					resign();
				}
			}

			bool isValid()
			{
				return (!(_job == nullptr || *_job == nullptr));
			}

			void resign()
			{
				if (isValid() == false)
					throw std::runtime_error("Can't resign an already resigned contract");

				*_job = nullptr;
				_originator->unsubscribe(*this);
				_job = nullptr;
			}

			void relinquish()
			{
				if (isValid() == false)
					throw std::runtime_error("Can't relinquish an already resigned contract");

				_originator->relinquish(std::move(*this));
				_job = nullptr;
				_originator = nullptr;
			}
		};
		using Job = Contract::Job;

	private:
		std::vector<std::shared_ptr<Job>> _subscribedJobs;
		std::vector<Contract> _relinquishedContracts;
		std::recursive_mutex _mutex;

	public:
		ContractProvider()
		{

		}

		~ContractProvider()
		{
			invalidateContracts();
		}

		void invalidateContracts()
		{
			for (auto& job : _subscribedJobs)
			{
				*job = nullptr;
			}
			_subscribedJobs.clear();
		}

		Contract subscribe(const Job& p_job)
		{
			std::shared_ptr<Job> toAdd = std::make_shared<Job>(p_job);

			std::lock_guard<std::recursive_mutex> lock(_mutex);
			_subscribedJobs.push_back(toAdd);

			return (Contract(this, toAdd));
		}

		void relinquish(Contract&& p_contract)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			_relinquishedContracts.push_back(std::move(p_contract));
		}

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