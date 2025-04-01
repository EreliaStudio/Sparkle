#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"

namespace spk
{
	template <typename TType>
	class ObservableValue
	{
	public:
		using Contract = ContractProvider::Contract;
		using Job = ContractProvider::Job;

	private:
		TType _value;
		ContractProvider _contractProvider;

	protected:
		void notifyEdition() const
		{
			_contractProvider.trigger();
		}

	public:
		ObservableValue() = default;

		ObservableValue(const TType &p_value) :
			_value(p_value)
		{
		}

		void trigger() const
		{
			notifyEdition();
		}

		ObservableValue &operator=(const TType &p_value)
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

		void set(const TType &p_value)
		{
			_value = p_value;
			notifyEdition();
		}

		Contract subscribe(const Job &p_job)
		{
			return _contractProvider.subscribe(p_job);
		}

		// Arithmetic assignment operators
		ObservableValue &operator+=(const TType &p_value)
		{
			_value += p_value;
			notifyEdition();
			return *this;
		}

		ObservableValue &operator-=(const TType &p_value)
		{
			_value -= p_value;
			notifyEdition();
			return *this;
		}

		ObservableValue &operator*=(const TType &p_value)
		{
			_value *= p_value;
			notifyEdition();
			return *this;
		}

		ObservableValue &operator/=(const TType &p_value)
		{
			_value /= p_value;
			notifyEdition();
			return *this;
		}

		ObservableValue &operator%=(const TType &p_value)
		{
			_value %= p_value;
			notifyEdition();
			return *this;
		}

		// Bitwise assignment operators
		ObservableValue &operator&=(const TType &p_value)
		{
			_value &= p_value;
			notifyEdition();
			return *this;
		}

		ObservableValue &operator|=(const TType &p_value)
		{
			_value |= p_value;
			notifyEdition();
			return *this;
		}

		ObservableValue &operator^=(const TType &p_value)
		{
			_value ^= p_value;
			notifyEdition();
			return *this;
		}

		ObservableValue &operator<<=(const TType &p_value)
		{
			_value <<= p_value;
			notifyEdition();
			return *this;
		}

		ObservableValue &operator>>=(const TType &p_value)
		{
			_value >>= p_value;
			notifyEdition();
			return *this;
		}
	};
}
