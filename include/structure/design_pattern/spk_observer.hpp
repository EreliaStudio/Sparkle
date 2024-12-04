#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"
#include <unordered_map>

namespace spk
{
	template <typename TType, typename... TArgs>
	class Observer
	{
	public:
		using Job = typename TContractProvider<TArgs...>::Job;
		using Contract = typename TContractProvider<TArgs...>::Contract;

	private:
		std::unordered_map<TType, TContractProvider<TArgs...>> _eventProviders;

	public:
		Observer() = default;
		virtual ~Observer() = default;

		Contract subscribe(const TType &p_event, const Job &p_job)
		{
			return _eventProviders[p_event].subscribe(p_job);
		}

		void invalidateContracts(const TType &p_event)
		{
			_eventProviders[p_event].invalidateContracts();
		}

		void notifyEvent(const TType &p_event, TArgs... args)
		{
			auto it = _eventProviders.find(p_event);
			if (it != _eventProviders.end())
			{
				it->second.trigger(args...);
			}
		}
	};
}
