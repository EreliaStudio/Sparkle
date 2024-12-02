#pragma once

#include <unordered_map>
#include <vector>
#include <functional>

#include "structure/design_pattern/spk_contract_provider.hpp"

namespace spk
{
	/**
	 * @class StatefulObject
	 * @brief A class that manages state transitions and triggers callbacks when the state changes.
	 * 
	 * The `StatefulObject` class is designed to associate specific states with callbacks. When the state of the object
	 * changes, the corresponding callbacks are triggered automatically. This is useful for implementing reactive behavior.
	 * 
	 * Example usage:
	 * @code
	 * spk::StatefulObject<std::string> stateful("InitialState");
	 * 
	 * stateful.addCallback("NewState", []() { std::cout << "State changed to NewState!" << std::endl; });
	 * stateful.setState("NewState"); // Triggers the callback
	 * @endcode
	 * 
	 * @tparam TType The type representing the state of the object.
	 */
	template <typename TType>
	class StatefulObject
	{
	public:
		using Contract = ContractProvider::Contract; /**< @brief Type alias for the callback subscription contract. */

	private:
		TType _state;
		std::unordered_map<TType, ContractProvider> _callbacks;

	public:
		/**
		 * @brief Constructs a `StatefulObject` with an initial state.
		 * 
		 * @param p_initialState The initial state of the object.
		 */
		StatefulObject(const TType& p_initialState) :
			_state(p_initialState)
		{
		}

		/**
		 * @brief Destructor.
		 */
		virtual ~StatefulObject()
		{
		}

		/**
		 * @brief Sets the state of the object and triggers associated callbacks.
		 * 
		 * If the new state is different from the current state, all registered callbacks for the new state are triggered.
		 * 
		 * @param newState The new state to set.
		 */
		void setState(const TType& newState)
		{
			if (_state != newState)
			{
				_state = newState;
				_callbacks[_state].trigger();
			}
		}

		/**
		 * @brief Gets the current state of the object.
		 * 
		 * @return The current state of the object.
		 */
		TType state() const
		{
			return _state;
		}

		/**
		 * @brief Adds a callback for a specific state.
		 * 
		 * The callback will be triggered whenever the object transitions to the specified state.
		 * 
		 * @param state The state for which the callback is registered.
		 * @param callback The function to call when the state is reached.
		 * @return A `Contract` object that can be used to manage the callback subscription.
		 */
		Contract addCallback(const TType& state, const std::function<void()>& callback)
		{
			return (std::move(_callbacks[state].subscribe(callback)));
		}
	};
}
