#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "core/platform/timer.hpp"
#include "exception.hpp"
#include "threading/task.hpp"

namespace spk
{
	// All methods other than Task::Answer completion are called on the owning
	// thread. No business callback is subscribed to the worker's completion.
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
			TimeoutCallback onTimeout;
			bool valid = true;
		};

		std::list<Entry> _entries;
		Id _lastId = 0;
		bool _collecting = false;

	public:
		[[nodiscard]] Id watch(Answer answer, std::unique_ptr<Timer> timer, TimeoutCallback onTimeout = {})
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
			_entries.push_back(Entry{id, std::move(answer), std::move(timer), std::move(onTimeout)});
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

		// Polling and callbacks occur only on the caller's thread. Completed
		// entries are removed before dispatch, so callbacks can add new work.
		template <typename TCallback>
		void collect(TCallback &&onSettled)
		{
			if (_collecting)
			{
				return;
			}
			_collecting = true;
			try
			{
				std::vector<Id> snapshot;
				snapshot.reserve(_entries.size());
				for (const Entry &entry : _entries)
				{
					snapshot.push_back(entry.id);
				}

				for (Id id : snapshot)
				{
					auto it = std::find_if(_entries.begin(), _entries.end(), [id](const Entry &entry) {
						return entry.id == id;
					});
					if (it == _entries.end())
					{
						continue;
					}

					if (it->answer.status() != Status::Pending)
					{
						const bool valid = it->valid;
						Answer answer = it->answer;
						_entries.erase(it);
						if (valid)
						{
							onSettled(id, answer);
						}
					}
					else if (it->valid && it->timer->state() == Timer::State::TimedOut)
					{
						it->valid = false;
						TimeoutCallback callback = std::move(it->onTimeout);
						if (callback)
						{
							callback(id);
						}
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
