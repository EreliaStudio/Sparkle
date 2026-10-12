#include "network/transfer.hpp"

#include <atomic>
#include <mutex>
#include <utility>

namespace spk
{
	struct Transfer::Data
	{
		std::atomic<ID> id = InvalidID;
		mutable std::mutex mutex;
		Status status = Status::Pending;
		Reason reason = Reason::None;
		CompletionProvider completionProvider;
	};

	Transfer::Transfer() :
		_data(std::make_shared<Data>())
	{
	}

	bool Transfer::operator==(const Transfer &other) const noexcept
	{
		return _data == other._data;
	}

	Transfer::ID Transfer::id() const noexcept
	{
		return _data->id.load();
	}

	Transfer::Status Transfer::status() const noexcept
	{
		const std::scoped_lock lock(_data->mutex);
		return _data->status;
	}

	Transfer::Reason Transfer::reason() const noexcept
	{
		const std::scoped_lock lock(_data->mutex);
		return _data->reason;
	}

	bool Transfer::isComplete() const noexcept
	{
		return status() != Status::Pending;
	}

	Transfer::CompletionContract Transfer::subscribeToCompletion(CompletionCallback callback)
	{
		auto once = std::make_shared<std::atomic_bool>(false);
		auto notify = [callback = std::move(callback), once](Status status, Reason reason) {
			if (!once->exchange(true))
			{
				callback(status, reason);
			}
		};
		Status currentStatus;
		Reason currentReason;
		CompletionContract contract;
		{
			const std::scoped_lock lock(_data->mutex);
			contract = _data->completionProvider.subscribe(notify);
			currentStatus = _data->status;
			currentReason = _data->reason;
		}
		if (currentStatus != Status::Pending)
		{
			notify(currentStatus, currentReason);
		}
		return contract;
	}

	void Transfer::_assignID(ID id) noexcept
	{
		if (id == InvalidID)
		{
			return;
		}
		ID expected = InvalidID;
		(void)_data->id.compare_exchange_strong(expected, id);
	}

	bool Transfer::_complete(Status status, Reason reason) noexcept
	{
		if (status == Status::Pending)
		{
			return false;
		}
		{
			const std::scoped_lock lock(_data->mutex);
			if (_data->status != Status::Pending)
			{
				return false;
			}
			_data->status = status;
			_data->reason = status == Status::Success ? Reason::None : reason;
		}
		try
		{
			_data->completionProvider.trigger(status, status == Status::Success ? Reason::None : reason);
		} catch (...)
		{
		}
		return true;
	}
}
