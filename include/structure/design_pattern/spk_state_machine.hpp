#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include <stdexcept>
#include <functional>

#include "utils/spk_string_utils.hpp"

namespace spk
{
	class StateMachine
	{
	public:
		class Step
		{
			friend class StateMachine;

		public:
			using ID = std::wstring;

			virtual ~Step() = default;
			virtual void onStart() = 0;
			virtual void onPending() = 0;
			virtual Step::ID onFinish() = 0;

			void reset() { _isStarted = false; _isFinished = false; }
			bool isStarted() const noexcept { return _isStarted; }
			bool isFinished() const noexcept { return _isFinished; }
			void finish() noexcept { _isFinished = true; }

		private:
			bool _isStarted = false;
			bool _isFinished = false;
		};

	private:
		std::unordered_map<Step::ID, std::unique_ptr<Step>> _steps;
		Step::ID _currentID;
		Step* _current = nullptr;
		mutable std::recursive_mutex _mutex;

	public:
		void clear()
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			_steps.clear();
			_current = nullptr;
		}

		bool contains(const Step::ID& p_id) const noexcept
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			return _steps.find(p_id) != _steps.end();
		}

		void addStep(const Step::ID& p_id, std::unique_ptr<Step>&& p_step)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
		
			if (contains(p_id) == true)
			{
				throw std::runtime_error("Step with this ID already exists.");
			}
		
			_steps.emplace(p_id, std::move(p_step));
		}

		void setStep(const Step::ID& p_id)
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

		Step::ID currentStepID() const
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			if (_current == nullptr)
			{
				throw std::runtime_error("No current step set.");
			}
			
			return _currentID;
		}

		void resetStep()
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			if (_current != nullptr)
			{
				_current->reset();
			}
		}

		void stop()
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			if (_current != nullptr)
			{
				_current->_isStarted = false;
				_current->_isFinished = false;
				_current = nullptr;
			}
		}

		void update()
		{
			std::unique_lock<std::recursive_mutex> lock(_mutex);
			if (_current == nullptr)
			{
				return;
			}

			Step* current = _current;

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

		bool isActive() const noexcept
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			return _current != nullptr;
		}
	};

	struct DefaultStep : public StateMachine::Step
	{
		std::function<void()> onStartCallback;
		std::function<void()> onPendingCallback;
		std::function<StateMachine::Step::ID()> onFinishCallback;

		DefaultStep() = default;
		DefaultStep(std::function<void()> p_onStart,
					std::function<void()> p_onPending,
					std::function<StateMachine::Step::ID()> p_onFinish) :
			onStartCallback(std::move(p_onStart)),
			onPendingCallback(std::move(p_onPending)),
			onFinishCallback(std::move(p_onFinish))
		{
			
		}

		void onStart() override
		{
			if(onStartCallback) onStartCallback();
		}

		void onPending() override
		{
			if(onPendingCallback) onPendingCallback();
		}

		Step::ID onFinish() override
		{
			return (onFinishCallback ? onFinishCallback() : L"");
		}
	};
}
