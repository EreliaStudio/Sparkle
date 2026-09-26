#pragma once

#include <atomic>
#include <concepts>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

#include <design_pattern/contract_provider.hpp>
#include <exception.hpp>

namespace spk
{
	template <typename TResult>
		requires std::movable<TResult>
	class Task
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
		std::shared_ptr<State> _state =
			std::make_shared<State>();

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

		void _ensurePendingLocked() const
		{
			if (
				_state->status.load(
					std::memory_order_relaxed) !=
				Status::Pending)
			{
				throw spk::Exception(
					"Task is already settled");
			}
		}

	public:
		Task() = default;
		Task(const Task &) = delete;
		Task(Task &&) noexcept = default;

		Task &operator=(const Task &) = delete;
		Task &operator=(Task &&) noexcept = default;

		[[nodiscard]] Answer answer() const
		{
			return Answer(_state);
		}

		void validate(TResult result)
		{
			{
				const std::scoped_lock lock(
					_state->completionMutex);
				_ensurePendingLocked();
				_state->result.emplace(
					std::move(result));
				_state->status.store(
					Status::Completed,
					std::memory_order_release);
			}

			_notifyCompletion();
		}

		void fail(std::exception_ptr failure)
		{
			if (failure == nullptr)
			{
				throw spk::Exception(
					"Task failure cannot be null");
			}

			{
				const std::scoped_lock lock(
					_state->completionMutex);
				_ensurePendingLocked();
				_state->failure = std::move(failure);
				_state->status.store(
					Status::Failed,
					std::memory_order_release);
			}

			_notifyCompletion();
		}
	};
}
