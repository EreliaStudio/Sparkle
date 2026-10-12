#include "network/connection.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <utility>

namespace spk
{
	struct Connection::Data
	{
		std::atomic<ID> id = InvalidID;
		mutable std::mutex mutex;
		Status status = Status::Disconnected;
		Error error = Error::None;
		std::atomic<std::size_t> maxQueuedBytes = 64u * 1024u * 1024u;
		StatusProvider statusProvider;
	};

	Connection::Connection() :
		_data(std::make_shared<Data>())
	{
	}

	bool Connection::operator==(const Connection &other) const noexcept
	{
		return _data == other._data;
	}

	std::size_t Connection::Hash::operator()(const Connection &connection) const noexcept
	{
		return std::hash<const void *>{}(connection._data.get());
	}

	Connection::ID Connection::id() const noexcept
	{
		return _data->id.load();
	}

	Connection::Status Connection::status() const noexcept
	{
		const std::scoped_lock lock(_data->mutex);
		return _data->status;
	}

	Connection::Error Connection::error() const noexcept
	{
		const std::scoped_lock lock(_data->mutex);
		return _data->error;
	}

	bool Connection::isConnected() const noexcept
	{
		return status() == Status::Connected;
	}

	std::size_t Connection::maxQueuedBytes() const noexcept
	{
		return _data->maxQueuedBytes.load();
	}

	void Connection::setMaxQueuedBytes(std::size_t bytes) noexcept
	{
		_data->maxQueuedBytes.store(bytes);
	}

	Connection::StatusContract Connection::subscribeToStatusChange(StatusCallback callback)
	{
		auto contract = _data->statusProvider.subscribe(callback);
		callback(status(), error());
		return contract;
	}

	void Connection::_assignID(ID id) noexcept
	{
		ID expected = InvalidID;
		(void)_data->id.compare_exchange_strong(expected, id);
	}

	void Connection::_setStatus(Status status, Error error)
	{
		{
			const std::scoped_lock lock(_data->mutex);
			if (_data->status == status)
			{
				return;
			}
			_data->status = status;
			_data->error = error;
		}
		_data->statusProvider.trigger(status, error);
	}
}
