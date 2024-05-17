#pragma once

#include <iostream>
#include <unordered_map>
#include <functional>

namespace spk
{
	/**
	 * @class StateMachine
	 *
	 * @tparam TStateType The type used to represent state identifiers (e.g., an enumeration).
	 * 
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
	 */
	template<typename TStateType>
	class StateMachine
	{
	public:

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
			/**
			 * @brief Describe the required callback type to provide.
			*/
			using Callback = std::function<void()>;

			Callback onEnter; //!< Callback called by the state machine when user require a transition to the state linked to this Action
			Callback onUpdate; //!< Callback called by the state machine when user require an update linked to this Action
			Callback onExit; //!< Callback called by the state machine when user require a transition from the state linked to this Action

			/**
			 * @brief Default constructor, setting every callback to nullptr
			*/
			Action() :
				onEnter(nullptr),
				onUpdate(nullptr),
				onExit(nullptr)
			{

			}

			/**
			 * @brief Constructor with only one callback, used as update callback
			 * 
			 * @param p_callback The callback used as update.
			*/
			Action(const Callback& p_callback) :
				onEnter(nullptr),
				onUpdate(p_callback),
				onExit(nullptr)
			{

			}


			/**
			 * @brief Constructor with three callbacks, used as entry/update/exit callbacks
			 * 
			 * @param p_entryCallback The callback used as update.
			 * @param p_onUpdateCallback The callback used as update.
			 * @param p_exitCallback The callback used as update.
			*/
			Action(const Callback& p_entryCallback, const Callback& p_onUpdateCallback, const Callback& p_exitCallback) :
				onEnter(p_entryCallback),
				onUpdate(p_onUpdateCallback),
				onExit(p_exitCallback)
			{

			}
		};

	private:

		std::unordered_map<TStateType, Action> _states;
		TStateType _currentState;
		Action* _currentAction;

	public:
		/**
		 * @brief Constructor with initial state and Action struct.
		 * 
		 * Initializes the StateMachine with a single state defined by an Action struct,
		 * which includes callbacks for entering, updating, and exiting the state. Automatically
		 * enters the specified state.
		 * 
		 * @param p_stateID Initial state identifier.
		 * @param p_action Action struct containing the callbacks for the state.
		 */
		StateMachine(TStateType p_stateID, const Action& p_action) :
			_currentAction(nullptr)
		{
			addState(p_stateID, p_action);
			enterState(p_stateID);
		}

		/**
		 * @brief Default constructor.
		 * 
		 * Initializes the StateMachine without any initial state. States must be added
		 * using addState before the StateMachine can be used.
		 */
		StateMachine() :
			_currentAction(nullptr)
		{

		}
		
		/**
		 * @brief Retrieves the action associated with a given state identifier.
		 * 
		 * Provides direct access to the Action struct for a specified state, allowing modification
		 * of the callbacks. Throws std::out_of_range if the state does not exist.
		 * 
		 * @param p_stateID State identifier.
		 * @return A reference to the Action struct associated with the state.
		 */
		Action& action(TStateType p_stateID)
		{
			if (!_states.contains(p_stateID))
			{
				throw std::out_of_range("State ID [" + std::to_string(static_cast<int>(p_stateID)) + "] not found");
			}
			return _states.at(p_stateID);
		}

		/**
		 * @brief Retrieves the action associated with a given state identifier (const version).
		 * 
		 * Provides read-only access to the Action struct for a specified state. Throws std::out_of_range
		 * if the state does not exist.
		 * 
		 * @param p_stateID State identifier.
		 * @return A const reference to the Action struct associated with the state.
		 */
		const Action& action(TStateType p_stateID) const
		{
			if (!_states.contains(p_stateID))
			{
				throw std::out_of_range("State ID [" + std::to_string(static_cast<int>(p_stateID)) + "] not found");
			}
			return _states[p_stateID];
		}
		
		/**
		 * @brief Retrieves the action of the current state.
		 * 
		 * Provides direct access to the Action struct for the current state, allowing modification
		 * of the callbacks. Useful for dynamically changing the behavior of the current state.
		 * 
		 * @return A reference to the Action struct for the current state.
		 */
		Action& currentAction()
		{
			return (_currentAction);
		}

		/**
		 * @brief Retrieves the action of the current state (const version).
		 * 
		 * Provides read-only access to the Action struct for the current state.
		 * 
		 * @return A const reference to the Action struct for the current state.
		 */
		const Action& currentAction() const
		{
			return (_currentAction);
		}

		/**
		 * @brief Adds a state defined by an Action struct.
		 * 
		 * Registers a new state in the StateMachine using an Action struct, which includes callbacks
		 * for entering, updating, and exiting the state. Throws an exception if the state already exists.
		 * 
		 * @param p_stateID State identifier.
		 * @param actions Action struct containing the callbacks for the state.
		 */
		void addState(TStateType p_stateID, const Action& actions)
		{
			if (_states.contains(p_stateID))
			{
				throw std::runtime_error("State ID [" + std::to_string(static_cast<int>(p_stateID)) + "] already defined");
			}
			_states[p_stateID] = actions;
		}

		/**
		 * @brief Transitions to a specified state.
		 * 
		 * Changes the current state to the specified state, executing the exit action of the current
		 * state (if any) and the enter action of the new state (if any).
		 * 
		 * @param p_newState The state identifier to transition to.
		 */
		void enterState(TStateType p_newState)
		{
			if (_currentAction != nullptr && _currentAction->onExit != nullptr)
			{
				_currentAction->onExit();
			}

			_currentState = p_newState;

			if (_states.contains(p_newState) == false)
				_currentAction = nullptr;
			else
				_currentAction = &(_states.at(p_newState));


			if (_currentAction != nullptr && _currentAction->onEnter != nullptr)
			{
				_currentAction->onEnter();
			}
		}

		/**
		 * @brief Executes the update action of the current state.
		 * 
		 * Calls the update callback associated with the current state. Does nothing if there
		 * is no current state or if the current state has no update callback.
		 */
		void update()
		{
			if (_currentAction != nullptr && _currentAction->onUpdate != nullptr) 
			{
				_currentAction->onUpdate();
			}
		}

		/**
		 * @brief Retrieves a copy of the current state.
		 * 
		 * Provides access to the current state.
		 * 
		 * @return A copy of the current state.
		 */
		TStateType state() const
		{
			return (_currentState);
		}
	};
}