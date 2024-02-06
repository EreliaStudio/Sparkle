#pragma once

#include <iostream>
#include <unordered_map>
#include <functional>

namespace spk
{
	/**
	 * @brief A generic state machine for managing states and associated actions.
	 * 
	 * The StateMachine class provides a flexible mechanism for managing states and their corresponding
	 * actions within an application. It allows developers to define states with custom actions for entering,
	 * updating, and exiting each state. State transitions can be triggered using the `enterState` method.
	 *
	 * This class is designed to be used as a foundation for implementing state-driven behavior in various
	 * components of an application, such as game characters, user interfaces, or finite state machines.
	 *
	 * Usage example:
	 * @code
	 * spk::StateMachine<MyStateEnum> stateMachine;
	 * stateMachine.addState(MyStateEnum::Idle, []{ /* Uppon update state callback * / });
	 * stateMachine.addState(MyStateEnum::Running, []{ /* Enter state callback * / }, []{ /* Uppon update state callback * / }, []{ /* Exiting state callback * / });
	 * stateMachine.enterState(MyStateEnum::Idle); // Transition to the "Idle" state
	 * stateMachine.update(); // Execute state-specific update action
	 * @endcode
	 *
	 * @tparam TStateType The type used to represent state identifiers (e.g., an enumeration).
	 */
	template<typename TStateType>
	class StateMachine
	{
	public:
		using Callback = std::function<void()>;

		/**
		 * @brief Represents a collection of actions associated with a state in a StateMachine.
		 * 
		 * The Action struct encapsulates the actions associated with a state in a StateMachine. It defines
		 * three types of actions: onEnter (executed when entering the state), onUpdate (executed during state updates),
		 * and onExit (executed when exiting the state). Actions can be specified as function objects (std::function<void()>).
		 *
		 * This struct allows developers to define custom behavior for different states, enabling precise control
		 * over the actions to be taken when transitioning between states within a StateMachine.
		 *
		 * Usage example:
		 * @code
		 * spk::StateMachine<MyStateEnum>::Action action;
		 * action.onEnter = []{ /* Enter state actions * / };
		 * action.onUpdate = []{ /* Update state actions * / };
		 * action.onExit = []{ /* Exit state actions * / };
		 * @endcode
		 * 
		 * @see StateMachine
		 */
		struct Action
		{
			Callback onEnter;
			Callback onUpdate;
			Callback onExit;
		};

	private:

		std::unordered_map<TStateType, Action> _states;
		Action _currentActions;
		bool _hasCurrentState = false;

	public:
		StateMachine(TStateType p_stateID, Callback p_onEnter, Callback p_onUpdate, Callback p_onExit) :
			_currentState(p_stateID),
			_hasCurrentState(true)
		{
			addState(p_stateID, p_onEnter, p_onUpdate, p_onExit);
			enterState(p_stateID);
		}

		StateMachine(TStateType p_stateID, Callback p_onUpdate) :
			_currentState(p_stateID),
			_hasCurrentState(true)
		{
			addState(p_stateID, p_onUpdate);
			enterState(p_stateID);
		}

		StateMachine(TStateType p_stateID, const Action& p_action) :
			_currentState(p_stateID),
			_hasCurrentState(true)
		{
			addState(p_stateID, p_action);
			enterState(p_stateID);
		}

		StateMachine() :
			_hasCurrentState(false)
		{

		}
		
		Action& action(TStateType p_stateID)
        {
            if (!_states.contains(p_stateID))
            {
                throw std::out_of_range("State ID [" + std::to_string(static_cast<int>(p_stateID)) + "] not found");
            }
            return _states[p_stateID];
        }
		
		const Action& action(TStateType p_stateID) const
        {
            if (!_states.contains(p_stateID))
            {
                throw std::out_of_range("State ID [" + std::to_string(static_cast<int>(p_stateID)) + "] not found");
            }
            return _states[p_stateID];
        }
		
		Action& currentAction()
        {
            return (_currentActions);
        }
		
		const Action& currentAction() const
        {
            return (_currentActions);
        }

		void addState(TStateType p_stateID, Callback p_onEnter, Callback p_onUpdate, Callback p_onExit)
		{
			if (_states.contains(p_stateID) == true)
				throw std::runtime_error("State ID [" + std::to_string(static_cast<int>(p_stateID)) + "] already defined");
			_states[p_stateID] = {p_onEnter, p_onUpdate, p_onExit};
		}

		void addState(TStateType p_stateID, Callback p_onUpdate)
		{
			if (_states.contains(p_stateID) == true)
				throw std::runtime_error("State ID [" + std::to_string(static_cast<int>(p_stateID)) + "] already defined");
			_states[p_stateID] = {nullptr, p_onUpdate, nullptr};
		}

		void addState(TStateType p_stateID, const Action& actions)
        {
            if (_states.contains(p_stateID))
            {
                throw std::runtime_error("State ID [" + std::to_string(static_cast<int>(p_stateID)) + "] already defined");
            }
            _states[p_stateID] = actions;
        }

		void enterState(TStateType p_newState)
		{
			if (_hasCurrentState == true)
			{
				if (_currentActions.onExit != nullptr)
				{
					_currentActions.onExit();
				}
			}

			_hasCurrentState = true;
			_currentActions = _states[p_newState];

			if (_currentActions.onEnter != nullptr)
				_currentActions.onEnter();
		}

		void update()
		{
			if (_hasCurrentState == true && _currentActions.onUpdate != nullptr) 
			{
				_currentActions.onUpdate();
			}
		}
	};
}