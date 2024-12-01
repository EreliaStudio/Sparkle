#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"
#include <map>

namespace spk
{
	template<typename TType>
	class EventNotifier
	{
	public:
		using Contract = ContractProvider::Contract;

	private:
		std::map<TType, ContractProvider> _eventProviders;

	public:
		EventNotifier() = default;
		virtual ~EventNotifier() = default;

		ContractProvider::Contract subscribe(const TType& p_event, const ContractProvider::Job& p_job)
		{
			return _eventProviders[p_event].subscribe(p_job);
		}

		void invalidateContracts(const TType& p_event)
		{
			_eventProviders[p_event].invalidateContracts();
		}

		void notifyEvent(const TType& p_event)
		{
			auto it = _eventProviders.find(p_event);
			if (it != _eventProviders.end())
			{
				it->second.trigger();
			}
		}
	};
}