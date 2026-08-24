#pragma once

#include <optional>
#include <unordered_map>
#include <utility>

#include "design_pattern/contract_provider.hpp"

namespace spk
{
	template <typename TStateType>
	class StatefullTrait
	{
	public:
		using OnStateEditionContractProvider = ContractProvider<>;
		using OnStateEditionCallback = OnStateEditionContractProvider::callback_type;
		using OnStateEditionContract = OnStateEditionContractProvider::Contract;

		StatefullTrait() = default;

		const std::optional<TStateType> &state() const
		{
			return _currentState;
		}

		void setState(TStateType state)
		{
			if (_currentState == state)
			{
				return;
			}

			_currentState = state;
			if (auto it = _stateChangeTriggers.find(state); it != _stateChangeTriggers.end())
			{
				it->second.trigger();
			}
		}

		OnStateEditionContract subscribeToStateEnter(TStateType state, OnStateEditionCallback callback)
		{
			return _stateChangeTriggers[state].subscribe(std::move(callback));
		}

	private:
		std::optional<TStateType> _currentState;
		std::unordered_map<TStateType, OnStateEditionContractProvider> _stateChangeTriggers;
	};
}
