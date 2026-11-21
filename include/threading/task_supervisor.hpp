#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <list>
#include <memory>
#include <utility>

#include "core/platform/timer.hpp"
#include "exception.hpp"
#include "threading/task.hpp"

namespace spk
{
	// Answers and timers are polled on the owning thread, never on workers.
	template <typename TResult>
	class TaskSupervisor
	{
	public:
		using Id = std::uint64_t;
		using Answer = typename Task<TResult>::Answer;
		using Status = typename Task<TResult>::Status;
		using TimeoutCallback = std::function<void(Id)>;

	private:
		struct Entry
		{
			Id id;
			Answer answer;
			std::unique_ptr<Timer> timer;
			bool valid = true;
		};

		std::list<Entry> _entries;
		Id _lastId = 0;
		bool _collecting = false;

	public:
		[[nodiscard]] Id watch(Answer answer, std::unique_ptr<Timer> timer)
		{
			if (timer == nullptr)
			{
				throw spk::Exception("TaskSupervisor requires a timer");
			}
			if (_lastId == std::numeric_limits<Id>::max())
			{
				throw spk::Exception("TaskSupervisor identifier exhausted");
			}
			timer->start();
			const Id id = ++_lastId;
			_entries.push_back(Entry{id, std::move(answer), std::move(timer)});
			return id;
		}

		void invalidate(Id id) noexcept
		{
			for (Entry &entry : _entries)
			{
				if (entry.id == id)
				{
					entry.valid = false;
					return;
				}
			}
		}

		void invalidateAll() noexcept
		{
			for (Entry &entry : _entries)
			{
				entry.valid = false;
			}
		}

		template <typename TFunction>
		void forEachTimer(TFunction &&function)
		{
			for (Entry &entry : _entries)
			{
				if (entry.valid && entry.answer.status() == Status::Pending)
				{
					function(*entry.timer);
				}
			}
		}

		// Only tasks present at entry are collected. Callbacks may add tasks.
		template <typename TCallback>
		void collect(TCallback &&onSettled, const TimeoutCallback &onTimeout = {})
		{
			if (_collecting || _entries.empty())
			{
				return;
			}
			_collecting = true;
			const Id lastId = _entries.back().id;
			try
			{
				for (auto it = _entries.begin(); it != _entries.end();)
				{
					auto current = it++;
					const bool last = current->id == lastId;

					if (current->answer.status() != Status::Pending)
					{
						const bool valid = current->valid;
						const Id id = current->id;
						Answer answer = current->answer;
						_entries.erase(current);
						if (valid)
						{
							onSettled(id, answer);
						}
					}
					else if (current->valid && current->timer->state() == Timer::State::TimedOut)
					{
						current->valid = false;
						if (onTimeout)
						{
							onTimeout(current->id);
						}
					}

					if (last)
					{
						break;
					}
				}
			} catch (...)
			{
				_collecting = false;
				throw;
			}
			_collecting = false;
		}

		[[nodiscard]] std::size_t inFlightCount() const noexcept
		{
			return _entries.size();
		}
	};
}
