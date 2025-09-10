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

		void setState(const TType &p_newState)
		{
			if (_state != p_newState)
			{
				_state = p_newState;
				_callbacks[_state].trigger();
			}
		}

		TType state() const
		{
			return _state;
		}

		Contract addCallback(const TType &p_state, const Job &p_callback)
		{
			return (std::move(_callbacks[p_state].subscribe(p_callback)));
		}
	};
}