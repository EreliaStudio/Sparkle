#include "network/connection.hpp"

#include <atomic>
#include <utility>

namespace spk
{
	struct Connection::Data
	{
		std::atomic<ID> id = InvalidID;
		std::atomic<Status> status = Status::Disconnected;
		std::atomic<Error> error = Error::None;
		std::atomic<std::size_t> maxQueuedBytes = 64u * 1024u * 1024u;
		StatusProvider statusProvider;
	};

	Connection::Connection() :
		_data(std::make_shared<Data>())
	{
	}

	Connection::ID Connection::id() const noexcept
	{
		return _data->id.load();
	}

	Connection::Status Connection::status() const noexcept
	{
		return _data->status.load();
	}

	Connection::Error Connection::error() const noexcept
	{
		return _data->error.load();
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
		return _data->statusProvider.subscribe(std::move(callback));
	}

	void Connection::_assignID(ID id) noexcept
	{
		_data->id.store(id);
	}

	void Connection::_setStatus(Status status, Error error)
	{
		if (_data->status.exchange(status) == status)
		{
			return;
		}
		_data->error.store(error);
		_data->statusProvider.trigger(status, error);
	}
}
