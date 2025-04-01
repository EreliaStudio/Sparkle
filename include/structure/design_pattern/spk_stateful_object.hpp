#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "structure/design_pattern/spk_contract_provider.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	template <typename TType>
	class StatefulObject
	{
	public:
		using Contract = ContractProvider::Contract;
		using Job = ContractProvider::Job;

	private:
		TType _state;
		std::unordered_map<TType, ContractProvider> _callbacks;

	public:
		StatefulObject(const TType &p_initialState) :
			_state(p_initialState)
		{
		}

		virtual ~StatefulObject()
		{
		}

		void setState(const TType &newState)
		{
			if (_state != newState)
			{
				_state = newState;
				_callbacks[_state].trigger();
			}
		}

		TType state() const
		{
			return _state;
		}

		Contract addCallback(const TType &state, const Job &callback)
		{
			return (std::move(_callbacks[state].subscribe(callback)));
		}
	};
}