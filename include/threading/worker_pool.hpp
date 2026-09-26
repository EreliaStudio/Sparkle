#pragma once

#include <concepts>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <container/thread_safe_queue.hpp>
#include <exception.hpp>

#include <threading/task.hpp>

namespace spk
{
	class WorkerPool final
	{
	public:
		class Job
		{
		private:
			virtual void _execute() noexcept = 0;

			friend class WorkerPool;

		public:
			virtual ~Job() = default;
		};

	private:
		template <typename TResult>
			requires std::movable<TResult>
		class TaskJob final :
			public Job,
			private Task<TResult>
		{
		private:
			std::move_only_function<TResult()> _operation;

			void _execute() noexcept override
			{
				try
				{
					this->validate(_operation());
				} catch (...)
				{
					try
					{
						this->fail(std::current_exception());
					} catch (...)
					{
						std::terminate();
					}
				}
			}

		public:
			template <typename TOperation>
				requires
					std::invocable<std::decay_t<TOperation> &> &&
					std::convertible_to<
						std::invoke_result_t<
							std::decay_t<TOperation> &>,
						TResult>
			explicit TaskJob(TOperation &&operation) :
				_operation(std::forward<TOperation>(operation))
			{
			}

			[[nodiscard]] typename Task<TResult>::Answer answer() const
			{
				return Task<TResult>::answer();
			}
		};

		template <typename TOperation>
		using OperationResult = std::remove_cvref_t<
			std::invoke_result_t<
				std::decay_t<TOperation> &>>;

		using JobQueue =
			ThreadSafeQueue<std::unique_ptr<Job>>;

		JobQueue _jobs;
		std::vector<std::jthread> _workers;

		[[nodiscard]] static std::size_t _defaultWorkerCount() noexcept
		{
			const unsigned int count =
				std::thread::hardware_concurrency();
			return count == 0u ?
					   1u :
					   static_cast<std::size_t>(count);
		}

		static void _run(
			JobQueue::Consumer consumer,
			std::stop_token stopToken)
		{
			while (auto job = consumer.waitPop(stopToken))
			{
				(*job)->_execute();
			}
		}

	public:
		WorkerPool() :
			WorkerPool(_defaultWorkerCount())
		{
		}

		explicit WorkerPool(std::size_t workerCount)
		{
			if (workerCount == 0u)
			{
				throw spk::Exception(
					"WorkerPool requires at least one worker");
			}

			_workers.reserve(workerCount);
			for (std::size_t index = 0; index < workerCount; ++index)
			{
				_workers.emplace_back(
					[consumer = _jobs.consumer()](
						std::stop_token stopToken) mutable {
						_run(
							std::move(consumer),
							stopToken);
					});
			}
		}

		WorkerPool(const WorkerPool &) = delete;
		WorkerPool(WorkerPool &&) = delete;

		WorkerPool &operator=(const WorkerPool &) = delete;
		WorkerPool &operator=(WorkerPool &&) = delete;

		~WorkerPool() = default;

		template <typename TOperation>
			requires
				std::invocable<std::decay_t<TOperation> &> &&
				std::movable<OperationResult<TOperation>>
		[[nodiscard]] auto submit(TOperation &&operation)
		{
			using TResult =
				OperationResult<TOperation>;

			auto task =
				std::make_unique<TaskJob<TResult>>(
					std::forward<TOperation>(operation));
			auto answer = task->answer();

			_jobs.publish(std::move(task));

			return answer;
		}

		[[nodiscard]] std::size_t workerCount() const noexcept
		{
			return _workers.size();
		}
	};
}
