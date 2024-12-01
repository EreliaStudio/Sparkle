#pragma once

#include "structure/thread/spk_thread.hpp"

#include "structure/design_pattern/spk_contract_provider.hpp"

namespace spk
{
	class PersistantWorker : public spk::Thread
	{
	public:
		using Job = spk::ContractProvider::Job;
		using Contract = spk::ContractProvider::Contract;

	private:
		std::atomic<bool> _running;

		spk::ContractProvider _preparationJobs;
		spk::ContractProvider _executionJobs;

	public:
		PersistantWorker(const std::wstring& p_name) :
			spk::Thread(p_name, [&]()
				{
					this->_running = true;
					_preparationJobs.trigger();
					while (this->_running.load() == true)
					{
						_executionJobs.trigger();
					}
				})
		{

		}

		~PersistantWorker()
		{
			if (this->_running.load() == true)
				stop();
			if (isJoinable() == true)
				join();
		}

		void stop()
		{
			this->_running = false;
		}

		void join()
		{
			if (this->_running.load() == true)
				stop();
			Thread::join();
		}

		Contract addPreparationStep(const Job& p_job)
		{
			return (_preparationJobs.subscribe(p_job));
		}

		Contract addExecutionStep(const Job& p_job)
		{
			return (_executionJobs.subscribe(p_job));
		}

		spk::ContractProvider& preparationJobs()
		{
			return (_preparationJobs);
		}

		spk::ContractProvider& executionJobs()
		{
			return (_executionJobs);
		}
	};
}