#pragma once

#include <atomic>
#include <concepts>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

#include <design_pattern/contract_provider.hpp>
#include <exception.hpp>

namespace spk
{
	class WorkerPool;

	template <typename TResult>
		requires std::movable<TResult>
	class Task final
	{
	public:
		enum class Status
		{
			Pending,
			Completed,
			Failed
		};

	private:
		using CompletionProvider = ContractProvider<>;

		struct State
		{
			std::atomic<Status> status = Status::Pending;
			std::optional<TResult> result;
			std::exception_ptr failure;
			std::mutex completionMutex;
			CompletionProvider completionProvider;
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

			friend class Task;

		public:
			[[nodiscard]] Status status() const noexcept
			{
				return _state->status.load(
					std::memory_order_acquire);
			}

			[[nodiscard]] const TResult &result() const
			{
				if (status() != Status::Completed)
				{
					throw spk::Exception(
						"Task result is not available");
				}
				return *_state->result;
			}

			[[nodiscard]] std::exception_ptr failure() const
			{
				if (status() != Status::Failed)
				{
					throw spk::Exception(
						"Task failure is not available");
				}
				return _state->failure;
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
		std::shared_ptr<State> _state;
		std::move_only_function<TResult()> _operation;

		void _notifyCompletion() noexcept
		{
			try
			{
				_state->completionProvider.trigger();
			} catch (...)
			{
			}
			_state->completionProvider.invalidate();
		}

		void _execute() noexcept
		{
			try
			{
				TResult result = _operation();

				{
					const std::scoped_lock lock(
						_state->completionMutex);
					_state->result.emplace(std::move(result));
					_state->status.store(
						Status::Completed,
						std::memory_order_release);
				}
			} catch (...)
			{
				const std::scoped_lock lock(
					_state->completionMutex);
				_state->failure = std::current_exception();
				_state->status.store(
					Status::Failed,
					std::memory_order_release);
			}

			_notifyCompletion();
		}

		friend class WorkerPool;

	public:
		template <typename TOperation>
			requires std::invocable<std::decay_t<TOperation> &>
		explicit Task(TOperation &&operation) :
			_state(std::make_shared<State>()),
			_operation(std::forward<TOperation>(operation))
		{
			static_assert(
				std::convertible_to<
					std::invoke_result_t<std::decay_t<TOperation> &>,
					TResult>);
		}

		Task(const Task &) = delete;
		Task(Task &&) noexcept = default;

		Task &operator=(const Task &) = delete;
		Task &operator=(Task &&) noexcept = default;

		[[nodiscard]] Answer answer() const
		{
			return Answer(_state);
		}
	};
}
