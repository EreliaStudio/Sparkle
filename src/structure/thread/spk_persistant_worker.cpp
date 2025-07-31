#include "structure/thread/spk_persistant_worker.hpp"

namespace spk
{
	PersistantWorker::PersistantWorker(const std::wstring &p_name) :
		spk::Thread(p_name,
			[&]()
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

	PersistantWorker::~PersistantWorker()
	{
		if (this->_running.load() == true)
		{
			stop();
		}
		if (isJoinable() == true)
		{
			join();
		}
	}

	void PersistantWorker::stop()
	{
		this->_running = false;
	}

	void PersistantWorker::join()
	{
		if (this->_running.load() == true)
		{
			stop();
		}
		Thread::join();
	}

	PersistantWorker::Contract PersistantWorker::addPreparationStep(const Job &p_job)
	{
		return (_preparationJobs.subscribe(p_job));
	}

	PersistantWorker::Contract PersistantWorker::addExecutionStep(const Job &p_job)
	{
		return (_executionJobs.subscribe(p_job));
	}

	spk::ContractProvider &PersistantWorker::preparationJobs()
	{
		return (_preparationJobs);
	}

	spk::ContractProvider &PersistantWorker::executionJobs()
	{
		return (_executionJobs);
	}
}
