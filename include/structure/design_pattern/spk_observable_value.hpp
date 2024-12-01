#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"

namespace spk
{
	template<typename TType>
	class ObservableValue
	{
	private:
		TType _value;
		ContractProvider _contractProvider;

	protected:
		void notifyEdition()
		{
			_contractProvider.trigger();
		}

	public:
		ObservableValue() = default;

		ObservableValue(const TType& p_value) :
			_value(p_value)
		{

		}

		ObservableValue& operator =(const TType& p_value)
		{
			set(p_value);
			return *this;
		}

		operator TType() const
		{
			return (_value);
		}

		TType get() const
		{
			return (_value);
		}

		void set(const TType& p_value)
		{
			_value = p_value;
			notifyEdition();
		}

		ContractProvider::Contract subscribe(const ContractProvider::Job& p_job)
		{
			return _contractProvider.subscribe(p_job);
		}
	};
}
