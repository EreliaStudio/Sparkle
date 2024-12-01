#pragma once

#include <vector>
#include <map>

#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/thread/spk_persistant_worker.hpp"

#include "structure/spk_safe_pointer.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	class Application
	{
	public:
		static constexpr const wchar_t* MainThreadName = L"MainThread";
		using Job = spk::ContractProvider::Job;
		using Contract = spk::ContractProvider::Contract;

		using PreparationJob = spk::ContractProvider::Job;
		using PreparationContract = spk::ContractProvider::Contract;

	private:
		std::atomic<bool> _isRunning;
		std::atomic<int> _errorCode;

		std::unordered_map<std::wstring, std::unique_ptr<spk::PersistantWorker>> _workers;
		spk::SafePointer<spk::PersistantWorker> _mainThreadWorker;

	public:
		Application();
		~Application();

		spk::SafePointer<spk::PersistantWorker> worker(const std::wstring& p_threadName);

		Contract addExecutionStep(const std::wstring& p_threadName, const Job& p_job);
		Contract addExecutionStep(const Job& p_job);

		PreparationContract addPreparationStep(const std::wstring& p_threadName, const PreparationJob& p_job);
		PreparationContract addPreparationStep(const PreparationJob& p_job);

		int run();

		void quit(int p_errorCode);

		bool isRunning() const;
	};
}