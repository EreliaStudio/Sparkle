#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
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
			std::recursive_mutex completionMutex;
			CompletionProvider completionProvider;
			std::vector<TaskAnswer> answers;
			std::vector<
				typename TaskAnswer::CompletionContract>
				completionContracts;
			std::size_t remaining = 0u;
			bool sealed = false;

			void childCompleted()
			{
				const std::scoped_lock lock(completionMutex);
				if (remaining == 0u)
				{
					return;
				}

				--remaining;
				tryComplete();
			}

			void tryComplete()
			{
				if (
					!sealed ||
					remaining != 0u ||
					status.load(std::memory_order_acquire) !=
						Status::Pending)
				{
					return;
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

				try
				{
					completionProvider.trigger();
				} catch (...)
				{
				}
				completionProvider.invalidate();
			}
		};

	public:
		class Answer final
		{
		public:
			using CompletionCallback =
				typename CompletionProvider::callback_type;

			class CompletionContract final
			{
			private:
				using Contract =
					typename CompletionProvider::Contract;

				std::shared_ptr<State> _state;
				std::optional<Contract> _contract;

				CompletionContract(
					std::shared_ptr<State> state,
					Contract contract) :
					_state(std::move(state)),
					_contract(std::move(contract))
				{
				}

				friend class Answer;

			public:
				CompletionContract() = default;
				~CompletionContract()
				{
					resign();
				}

				CompletionContract(
					const CompletionContract &) = delete;
				CompletionContract &operator=(
					const CompletionContract &) = delete;

				CompletionContract(
					CompletionContract &&other) noexcept :
					_state(std::move(other._state)),
					_contract(std::move(other._contract))
				{
				}

				CompletionContract &operator=(
					CompletionContract &&other) noexcept
				{
					if (this != &other)
					{
						resign();
						_state = std::move(other._state);
						_contract = std::move(other._contract);
					}
					return *this;
				}

				void resign() noexcept
				{
					if (_state == nullptr)
					{
						_contract.reset();
						return;
					}

					const std::scoped_lock lock(
						_state->completionMutex);
					_contract.reset();
					_state.reset();
				}

				[[nodiscard]] bool isValid() const noexcept
				{
					if (
						_state == nullptr ||
						!_contract.has_value())
					{
						return false;
					}

					const std::scoped_lock lock(
						_state->completionMutex);
					return _contract->isValid();
				}

				[[nodiscard]] explicit operator bool() const noexcept
				{
					return isValid();
				}
			};

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
				return _state->answers.size();
			}

			[[nodiscard]] const TaskAnswer &at(
				std::size_t index) const
			{
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
					const std::scoped_lock lock(
						_state->completionMutex);

					if (
						_state->status.load(
							std::memory_order_acquire) ==
						Status::Pending)
					{
						return CompletionContract(
							_state,
							_state->completionProvider.subscribe(
								std::move(callback)));
					}
				}

				callback();
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
			const std::scoped_lock lock(
				_state->completionMutex);

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
			const std::scoped_lock lock(
				_state->completionMutex);
			return _state->answers.size();
		}

		[[nodiscard]] Answer answer() &&
		{
			{
				const std::scoped_lock lock(
					_state->completionMutex);
				_state->sealed = true;
				_state->tryComplete();
			}

			return Answer(std::move(_state));
		}
	};
}
