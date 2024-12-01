#pragma once

#include <unordered_map>
#include <vector>
#include <functional>

#include "structure/design_pattern/spk_contract_provider.hpp"

namespace spk
{
	template <typename TType>
	class StatefulObject
	{
	public:
		using Contract = ContractProvider::Contract;

	private:
		TType _state;
		std::unordered_map<TType, ContractProvider> _callbacks;

	public:
		StatefulObject(const TType& p_initialState) :
			_state(p_initialState)
		{
		}

		virtual ~StatefulObject()
		{
		}

		void setState(const TType& newState)
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

		Contract addCallback(const TType& state, const std::function<void()>& callback)
		{
			return (std::move(_callbacks[state].subscribe(callback)));
		}
	};
}