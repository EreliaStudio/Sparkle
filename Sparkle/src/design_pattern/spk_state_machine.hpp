#pragma once

namespace spk
{
	#include <iostream>
	#include <unordered_map>
	#include <functional>

	template<typename TStateType>
	class StateMachine
	{
	public:
		using Action = std::function<void()>;

		struct StateActions
		{
			Action onEnter;
			Action onUpdate;
			Action onExit;
		};

	private:

		std::unordered_map<TStateType, StateActions> _states;
		StateActions _currentActions;
		bool _hasCurrentState = false;

	public:
		StateMachine(TStateType p_stateID, Action p_onEnter, Action p_onUpdate, Action p_onExit) :
			_currentState(p_stateID),
			_hasCurrentState(true)
		{
			addState(p_stateID, p_onEnter, p_onUpdate, p_onExit);
			enterState(p_stateID);
		}

		StateMachine(TStateType p_stateID, Action p_onUpdate) :
			_currentState(p_stateID),
			_hasCurrentState(true)
		{
			addState(p_stateID, p_onUpdate);
			enterState(p_stateID);
		}

		StateMachine(TStateType p_stateID, const StateActions& p_action) :
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
		
		StateActions& stateAction(TStateType p_stateID)
        {
            if (!_states.contains(p_stateID))
            {
                throw std::out_of_range("State ID [" + std::to_string(static_cast<int>(p_stateID)) + "] not found");
            }
            return _states[p_stateID];
        }
		
		const StateActions& stateAction(TStateType p_stateID) const
        {
            if (!_states.contains(p_stateID))
            {
                throw std::out_of_range("State ID [" + std::to_string(static_cast<int>(p_stateID)) + "] not found");
            }
            return _states[p_stateID];
        }
		
		StateActions& currentStateAction()
        {
            return (_currentActions);
        }
		
		const StateActions& currentStateAction() const
        {
            return (_currentActions);
        }

		void addState(TStateType p_stateID, Action p_onEnter, Action p_onUpdate, Action p_onExit)
		{
			if (_states.contains(p_stateID) == true)
				throw std::runtime_error("State ID [" + std::to_string(static_cast<int>(p_stateID)) + "] already defined");
			_states[p_stateID] = {p_onEnter, p_onUpdate, p_onExit};
		}

		void addState(TStateType p_stateID, Action p_onUpdate)
		{
			if (_states.contains(p_stateID) == true)
				throw std::runtime_error("State ID [" + std::to_string(static_cast<int>(p_stateID)) + "] already defined");
			_states[p_stateID] = {nullptr, p_onUpdate, nullptr};
		}

		void addState(TStateType p_stateID, const StateActions& actions)
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