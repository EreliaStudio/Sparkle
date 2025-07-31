#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>

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

			void reset()
			{
				_isStarted = false;
				_isFinished = false;
			}
			bool isStarted() const noexcept
			{
				return _isStarted;
			}
			bool isFinished() const noexcept
			{
				return _isFinished;
			}
			void finish() noexcept
			{
				_isFinished = true;
			}

		private:
			bool _isStarted = false;
			bool _isFinished = false;
		};

	private:
		std::unordered_map<Step::ID, std::unique_ptr<Step>> _steps;
		Step::ID _currentID;
		Step *_current = nullptr;
		mutable std::recursive_mutex _mutex;

	public:
		void clear();

		bool contains(const Step::ID &p_id) const noexcept;

		void addStep(const Step::ID &p_id, std::unique_ptr<Step> &&p_step);

		void setStep(const Step::ID &p_id);

		Step::ID currentStepID() const;

		void resetStep();

		void stop();

		void update();

		bool isActive() const noexcept;
	};

	struct DefaultStep : public StateMachine::Step
	{
		std::function<void()> onStartCallback;
		std::function<void()> onPendingCallback;
		std::function<StateMachine::Step::ID()> onFinishCallback;

		DefaultStep() = default;
		DefaultStep(std::function<void()> p_onStart, std::function<void()> p_onPending, std::function<StateMachine::Step::ID()> p_onFinish);

		void onStart() override;

		void onPending() override;

		Step::ID onFinish() override;
	};
}
