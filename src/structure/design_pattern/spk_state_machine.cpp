#include "structure/design_pattern/spk_state_machine.hpp"

namespace spk
{
	void StateMachine::clear()
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_steps.clear();
		_current = nullptr;
	}

	bool StateMachine::contains(const Step::ID &p_id) const noexcept
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return (_steps.find(p_id) != _steps.end());
	}

	void StateMachine::addStep(const Step::ID &p_id, std::unique_ptr<Step> &&p_step)
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		if (contains(p_id) == true)
		{
			throw std::runtime_error("Step with this ID already exists.");
		}

		_steps.emplace(p_id, std::move(p_step));
	}

	void StateMachine::setStep(const Step::ID &p_id)
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		auto it = _steps.find(p_id);
		if (it == _steps.end())
		{
			throw std::runtime_error("Step ID '" + spk::StringUtils::wstringToString(p_id) + "' not found.");
		}

		_current = it->second.get();
		if (_current != nullptr)
		{
			_current->reset();
		}
		_currentID = p_id;
	}

	StateMachine::Step::ID StateMachine::currentStepID() const
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		if (_current == nullptr)
		{
			throw std::runtime_error("No current step set.");
		}

		return (_currentID);
	}

	void StateMachine::resetStep()
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		if (_current != nullptr)
		{
			_current->reset();
		}
	}

	void StateMachine::stop()
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		if (_current != nullptr)
		{
			_current->_isStarted = false;
			_current->_isFinished = false;
			_current = nullptr;
		}
	}

	void StateMachine::update()
	{
		std::unique_lock<std::recursive_mutex> lock(_mutex);
		if (_current == nullptr)
		{
			return;
		}

		Step *current = _current;

		if (current->_isStarted == false)
		{
			current->_isStarted = true;
			lock.unlock();
			current->onStart();
			lock.lock();
		}

		lock.unlock();
		current->onPending();
		lock.lock();

		if (current->_isFinished == true)
		{
			Step::ID nextID = current->onFinish();
			lock.unlock();
			setStep(nextID);
		}
	}

	bool StateMachine::isActive() const noexcept
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return (_current != nullptr);
	}

	DefaultStep::DefaultStep(std::function<void()> p_onStart, std::function<void()> p_onPending, std::function<StateMachine::Step::ID()> p_onFinish) :
		onStartCallback(std::move(p_onStart)),
		onPendingCallback(std::move(p_onPending)),
		onFinishCallback(std::move(p_onFinish))
	{
	}

	void DefaultStep::onStart()
	{
		if (onStartCallback)
		{
			onStartCallback();
		}
	}

	void DefaultStep::onPending()
	{
		if (onPendingCallback)
		{
			onPendingCallback();
		}
	}

	StateMachine::Step::ID DefaultStep::onFinish()
	{
		return (onFinishCallback ? onFinishCallback() : L"");
	}
}
