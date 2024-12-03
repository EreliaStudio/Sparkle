#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"

namespace spk
{
	/**
	 * @class ObservableValue
	 * @brief A templated class that holds a value and notifies subscribers when the value changes.
	 * 
	 * The `ObservableValue` class allows you to store a value and register callbacks that are triggered
	 * whenever the value is updated. This is useful for implementing reactive systems.
	 * 
	 * Example usage:
	 * @code
	 * spk::ObservableValue<int> observable(42);
	 * auto contract = observable.subscribe([]() { std::cout << "Value changed!" << std::endl; });
	 * observable.set(100); // Outputs: Value changed!
	 * observable = 42; // Outputs: Value changed!
	 * observable.notifyEdition(); // Outputs: Value changed!
	 * @endcode
	 * 
	 * @tparam TType The type of the value held by the `ObservableValue`.
	 */
	template<typename TType>
	class ObservableValue
	{
	private:
		TType _value; /**< @brief The value held by the `ObservableValue`. */
		ContractProvider _contractProvider; /**< @brief Manages subscriptions for value change notifications. */

	protected:

	public:
		/**
		 * @brief Default constructor.
		 */
		ObservableValue() = default;

		/**
		 * @brief Constructs an `ObservableValue` with an initial value.
		 * 
		 * @param p_value The initial value.
		 */
		ObservableValue(const TType& p_value) :
			_value(p_value)
		{
		}

		/**
		 * @brief Assigns a new value to the `ObservableValue`.
		 * 
		 * This also triggers all subscribed callbacks to notify about the value change.
		 * 
		 * @param p_value The new value to assign.
		 * @return A reference to the current `ObservableValue`.
		 */
		ObservableValue& operator=(const TType& p_value)
		{
			set(p_value);
			return *this;
		}

		/**
		 * @brief Implicit conversion operator to the stored value type.
		 * 
		 * Allows the `ObservableValue` to be used wherever the underlying type is expected.
		 * 
		 * @return The current value.
		 */
		operator TType() const
		{
			return _value;
		}

		/**
		 * @brief Retrieves the current value.
		 * 
		 * @return The current value.
		 */
		TType get() const
		{
			return _value;
		}

		/**
		 * @brief Updates the value and triggers notifications for subscribers.
		 * 
		 * @param p_value The new value to set.
		 */
		void set(const TType& p_value)
		{
			_value = p_value;
			notifyEdition();
		}

		/**
		 * @brief Subscribes a callback to be triggered whenever the value changes.
		 * 
		 * @param p_job The callback function to subscribe.
		 * @return A `Contract` object to manage the subscription lifecycle.
		 */
		ContractProvider::Contract subscribe(const ContractProvider::Job& p_job)
		{
			return _contractProvider.subscribe(p_job);
		}

		/**
		 * @brief Triggers all subscribed callbacks to notify about a value change.
		 */
		void notifyEdition()
		{
			_contractProvider.trigger();
		}
	};
}
