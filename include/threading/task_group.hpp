#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory>
#include <mutex>
#include <span>
#include <utility>
#include <vector>

#include <design_pattern/contract_provider.hpp>
#include <exception.hpp>
#include <threading/task.hpp>

namespace spk
{
	template <typename TResult>
		requires std::movable<TResult>
	class TaskGroup final
	{
	public:
		using TaskType = Task<TResult>;
		using TaskAnswer = typename TaskType::Answer;
		using Status = typename TaskType::Status;

	private:
		using CompletionProvider = ContractProvider<>;

		struct State final :
			std::enable_shared_from_this<State>
		{
			std::atomic<Status> status = Status::Pending;
			std::recursive_mutex mutex;
			CompletionProvider completionProvider;
			std::vector<TaskAnswer> answers;
			std::vector<
				typename TaskAnswer::CompletionContract>
				completionContracts;
			std::size_t remaining = 0u;
			bool sealed = false;

			[[nodiscard]] bool tryCompleteLocked()
			{
				if (
					!sealed ||
					remaining != 0u ||
					status.load(std::memory_order_acquire) !=
						Status::Pending)
				{
					return false;
				}

				Status finalStatus = Status::Completed;
				for (const TaskAnswer &answer : answers)
				{
					if (answer.status() == Status::Failed)
					{
						finalStatus = Status::Failed;
						break;
					}
				}

				status.store(
					finalStatus,
					std::memory_order_release);
				return true;
			}

			void notifyCompletion() noexcept
			{
				completionProvider.trigger();
				completionProvider.invalidate();
			}

			void childCompleted()
			{
				bool shouldNotify = false;
				{
					const std::scoped_lock lock(mutex);
					if (remaining == 0u)
					{
						return;
					}

					--remaining;
					shouldNotify = tryCompleteLocked();
				}

				if (shouldNotify)
				{
					notifyCompletion();
				}
			}
		};

	public:
		class Answer final
		{
		public:
			using CompletionCallback =
				typename CompletionProvider::callback_type;
			using CompletionContract =
				typename CompletionProvider::Contract;

		private:
			std::shared_ptr<State> _state;

			explicit Answer(std::shared_ptr<State> state) :
				_state(std::move(state))
			{
			}

			friend class TaskGroup;

		public:
			[[nodiscard]] Status status() const noexcept
			{
				return _state->status.load(
					std::memory_order_acquire);
			}

			[[nodiscard]] std::size_t size() const noexcept
			{
				const std::scoped_lock lock(_state->mutex);
				return _state->answers.size();
			}

			[[nodiscard]] const TaskAnswer &at(
				std::size_t index) const
			{
				const std::scoped_lock lock(_state->mutex);
				if (index >= _state->answers.size())
				{
					throw spk::Exception(
						"TaskGroup answer index is outside the group");
				}
				return _state->answers[index];
			}

			[[nodiscard]] std::span<const TaskAnswer> answers()
				const noexcept
			{
				return _state->answers;
			}

			[[nodiscard]] CompletionContract subscribeToCompletion(
				CompletionCallback callback) const
			{
				{
					const std::scoped_lock lock(_state->mutex);

					if (
						_state->status.load(
							std::memory_order_acquire) ==
						Status::Pending)
					{
						return _state->completionProvider.subscribe(
							[callback = std::move(callback)]() mutable {
								try
								{
									callback();
								} catch (...)
								{
								}
							});
					}
				}

				try
				{
					callback();
				} catch (...)
				{
				}

				return CompletionContract();
			}
		};

	private:
		std::shared_ptr<State> _state =
			std::make_shared<State>();

	public:
		TaskGroup() = default;
		TaskGroup(const TaskGroup &) = delete;
		TaskGroup(TaskGroup &&) noexcept = default;

		TaskGroup &operator=(const TaskGroup &) = delete;
		TaskGroup &operator=(TaskGroup &&) noexcept = default;

		void add(TaskAnswer answer)
		{
			const std::scoped_lock lock(_state->mutex);

			if (_state->sealed)
			{
				throw spk::Exception(
					"TaskGroup cannot add an Answer after it is sealed");
			}

			_state->answers.push_back(answer);
			++_state->remaining;

			const std::weak_ptr<State> weakState = _state;
			_state->completionContracts.push_back(
				answer.subscribeToCompletion(
					[weakState] {
						if (
							const std::shared_ptr<State> state =
								weakState.lock();
							state != nullptr)
						{
							state->childCompleted();
						}
					}));
		}

		[[nodiscard]] std::size_t size() const noexcept
		{
			const std::scoped_lock lock(_state->mutex);
			return _state->answers.size();
		}

		[[nodiscard]] Answer answer() &&
		{
			std::shared_ptr<State> state = std::move(_state);
			bool shouldNotify = false;
			{
				const std::scoped_lock lock(state->mutex);
				state->sealed = true;
				shouldNotify = state->tryCompleteLocked();
			}

			if (shouldNotify)
			{
				state->notifyCompletion();
			}

			return Answer(std::move(state));
		}
	};
}
