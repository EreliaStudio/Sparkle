#pragma once

#include <cstddef>
#include <memory>
#include <stop_token>
#include <thread>
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
		class TaskJob final : public Job
		{
		private:
			Task<TResult> _task;

			void _execute() noexcept override
			{
				_task._execute();
			}

		public:
			explicit TaskJob(Task<TResult> task) :
				_task(std::move(task))
			{
			}
		};

		using JobQueue = ThreadSafeQueue<std::unique_ptr<Job>>;

		JobQueue _jobs;
		std::vector<std::jthread> _workers;

		[[nodiscard]] static std::size_t _defaultWorkerCount() noexcept
		{
			const unsigned int count = std::thread::hardware_concurrency();
			return count == 0u ? 1u : static_cast<std::size_t>(count);
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

		template <typename TResult>
			requires std::movable<TResult>
		[[nodiscard]] typename Task<TResult>::Answer submit(
			Task<TResult> task)
		{
			auto answer = task.answer();
			_jobs.publish(
				std::make_unique<TaskJob<TResult>>(
					std::move(task)));
			return answer;
		}

		[[nodiscard]] std::size_t workerCount() const noexcept
		{
			return _workers.size();
		}
	};
}
