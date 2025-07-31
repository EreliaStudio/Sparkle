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
PersistantWorker(const std::wstring &p_name);
virtual ~PersistantWorker();

void stop();
void join();

Contract addPreparationStep(const Job &p_job);
Contract addExecutionStep(const Job &p_job);

spk::ContractProvider &preparationJobs();
spk::ContractProvider &executionJobs();
};
}