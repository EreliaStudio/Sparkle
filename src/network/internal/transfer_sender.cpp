#include "network/internal/transfer_sender.hpp"

#include <utility>

namespace spk::NetworkInternal
{
	spk::Transfer TransferSender::enqueue(Message message, Message::Priority priority)
	{
		return _pages.enqueueTracked(std::move(message), priority);
	}

	std::optional<Page> TransferSender::nextPage(std::size_t maxPageBytes)
	{
		return _pages.nextPage(maxPageBytes);
	}

	void TransferSender::finalPageWritten(std::uint64_t id, AcknowledgmentDeadlines::Clock::time_point now)
	{
		if (id == 0 || !_pages.isTracked(id))
		{
			return;
		}
		_deadlines.start(id, now);
		if (!_pages.isTracked(id))
		{
			_deadlines.forget(id);
		}
	}

	bool TransferSender::acknowledge(const TransferResponse &response)
	{
		const bool resolved = _pages.complete(response.transferID, response.status, response.reason);
		_deadlines.forget(response.transferID);
		return resolved;
	}

	void TransferSender::expire(AcknowledgmentDeadlines::Clock::time_point now)
	{
		for (const auto id : _deadlines.expire(now))
		{
			(void)_pages.complete(id, spk::Transfer::Status::Indeterminate, spk::Transfer::Reason::AcknowledgmentTimeout);
		}
	}

	void TransferSender::setAcknowledgmentTimeout(std::chrono::milliseconds timeout)
	{
		_deadlines.setTimeout(timeout);
	}

	std::size_t TransferSender::pendingTransfers() const noexcept
	{
		return _pages.pendingTransfers();
	}

	std::size_t TransferSender::pendingBytes() const noexcept
	{
		return _pages.pendingBytes();
	}

	void TransferSender::clear() noexcept
	{
		_deadlines.clear();
		_pages.clear();
		_deadlines.clear();
	}
}
