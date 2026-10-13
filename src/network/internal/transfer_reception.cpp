#include "network/internal/transfer_reception.hpp"

#include <utility>

namespace spk::NetworkInternal
{
	spk::Transfer::Reason TransferReception::_reason(PageReassemblyError::Reason reason) noexcept
	{
		using Source = PageReassemblyError::Reason;
		using Target = spk::Transfer::Reason;
		switch (reason)
		{
		case Source::DuplicatePage:
			return Target::DuplicatePage;
		case Source::ConflictingPage:
			return Target::ConflictingPage;
		case Source::OverlappingPage:
			return Target::OverlappingPage;
		case Source::InvalidOffset:
			return Target::InvalidOffset;
		case Source::InvalidLength:
			return Target::InvalidLength;
		case Source::InvalidFirstPage:
			return Target::InvalidFirstPage;
		case Source::InconsistentMetadata:
			return Target::InconsistentMetadata;
		case Source::Capacity:
			return Target::ReceiverCapacity;
		}
		return Target::TransportFailure;
	}

	TransferReception::Result TransferReception::accept(
		const PageFragment &page, Connection::ID connectionID, Clock::time_point now)
	{
		try
		{
			auto message = _reassembler.accept(page, now);
			if (!message.has_value())
			{
				return {};
			}
			return {std::move(message), TransferResponse{connectionID, page.transferID, spk::Transfer::Status::Success, spk::Transfer::Reason::None}};
		} catch (const PageReassemblyError &error)
		{
			if (_reassembler.failureLimitReached())
			{
				throw;
			}
			auto status = spk::Transfer::Status::Failed;
			if (error.reason() == PageReassemblyError::Reason::Capacity)
			{
				status = spk::Transfer::Status::Rejected;
			}
			return {std::nullopt, TransferResponse{connectionID, error.transferID(), status, _reason(error.reason())}};
		}
	}

	std::vector<TransferResponse> TransferReception::expire(Connection::ID connectionID, Clock::time_point now)
	{
		std::vector<TransferResponse> responses;
		for (const auto id : _reassembler.expire(now))
		{
			responses.push_back(TransferResponse{connectionID, id, spk::Transfer::Status::Failed, spk::Transfer::Reason::InactivityTimeout});
		}
		return responses;
	}

	void TransferReception::setInactivityTimeout(std::chrono::milliseconds timeout)
	{
		_reassembler.setInactivityTimeout(timeout);
	}

	void TransferReception::configureFailureLimits(PageFailureLimits limits)
	{
		_reassembler.configureFailureLimits(limits);
	}

	bool TransferReception::failureLimitReached() const
	{
		return _reassembler.failureLimitReached();
	}

	void TransferReception::successfulMessage()
	{
		_reassembler.successfulMessage();
	}

	void TransferReception::clear() noexcept
	{
		_reassembler.clear();
	}

	std::size_t TransferReception::reservedBytes() const noexcept
	{
		return _reassembler.reservedBytes();
	}
}
