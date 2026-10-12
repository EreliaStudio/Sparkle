#include "network/internal/page_reassembler.hpp"

#include "exception.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace spk::NetworkInternal
{
	PageReassembler::Transfer::Transfer(const PageFragment &first, Clock::time_point now) :
		writer(first.header.messageType),
		updated(now)
	{
		writer.setRequestID(first.header.requestID);
		writer.setConnectionID(first.header.connectionID);
		writer.resize(static_cast<std::size_t>(first.totalSize));
	}

	PageReassembler::PageReassembler(
		std::size_t maxMessageBytes,
		std::size_t maxIncompleteBytes,
		std::chrono::milliseconds timeout) :
		_maxMessageBytes(maxMessageBytes),
		_maxIncompleteBytes(maxIncompleteBytes),
		_timeout(timeout)
	{
		if (timeout.count() <= 0)
		{
			throw Exception("Reassembly timeout must be positive.");
		}
	}

	void PageReassembler::_validateNew(const PageFragment &page) const
	{
		using Reason = PageReassemblyError::Reason;
		if (page.transferID == 0)
		{
			throw Exception("Network page has an invalid transfer ID.");
		}
		if (page.totalSize > _maxMessageBytes || page.totalSize > std::numeric_limits<std::size_t>::max())
		{
			throw PageReassemblyError(page.transferID, Reason::InvalidLength);
		}
		if (page.offset > page.totalSize || page.data.size() > page.totalSize - page.offset ||
			(page.totalSize != 0 && page.data.empty()))
		{
			throw PageReassemblyError(page.transferID, Reason::InvalidLength);
		}
		if (page.offset != 0)
		{
			throw PageReassemblyError(page.transferID, Reason::InvalidFirstPage);
		}
		if (_transfers.size() >= 512 ||
			page.totalSize > _maxIncompleteBytes - std::min(_reservedBytes, _maxIncompleteBytes))
		{
			throw PageReassemblyError(page.transferID, Reason::Capacity);
		}
	}

	bool PageReassembler::_duplicate(const Transfer &transfer, const PageFragment &page) const
	{
		const auto iterator = transfer.portions.find(page.offset);
		if (iterator == transfer.portions.end() || iterator->second != page.data.size())
		{
			return false;
		}
		const auto bytes = transfer.writer.data().subspan(
			static_cast<std::size_t>(page.offset), page.data.size());
		return std::equal(bytes.begin(), bytes.end(), page.data.begin());
	}

	void PageReassembler::_insert(Transfer &transfer, const PageFragment &page, Clock::time_point now)
	{
		using Reason = PageReassemblyError::Reason;
		if (page.totalSize != transfer.writer.size() ||
			page.header.messageType != transfer.writer.type() ||
			page.header.requestID != transfer.writer.requestID() ||
			page.header.connectionID != transfer.writer.connectionID())
		{
			throw PageReassemblyError(page.transferID, Reason::InconsistentMetadata);
		}
		if (page.offset > page.totalSize || page.data.size() > page.totalSize - page.offset)
		{
			throw PageReassemblyError(page.transferID, Reason::InvalidOffset);
		}
		if (page.totalSize != 0 && page.data.empty())
		{
			throw PageReassemblyError(page.transferID, Reason::InvalidLength);
		}
		auto next = transfer.portions.lower_bound(page.offset);
		if (next != transfer.portions.end() && next->first == page.offset)
		{
			throw PageReassemblyError(
				page.transferID, _duplicate(transfer, page) ? Reason::DuplicatePage : Reason::ConflictingPage);
		}
		if ((next != transfer.portions.end() && page.data.size() > next->first - page.offset) ||
			(next != transfer.portions.begin() &&
			 std::prev(next)->second > page.offset - std::prev(next)->first))
		{
			throw PageReassemblyError(page.transferID, Reason::OverlappingPage);
		}
		std::copy(page.data.begin(), page.data.end(), transfer.writer.data().begin() + page.offset);
		transfer.portions.emplace(page.offset, page.data.size());
		transfer.receivedBytes += page.data.size();
		transfer.updated = now;
	}

	std::optional<Message> PageReassembler::accept(const PageFragment &page, Clock::time_point now)
	{
		if (_failedTransfers.contains(page.transferID))
		{
			return std::nullopt;
		}
		auto iterator = _transfers.find(page.transferID);
		try
		{
			if (iterator == _transfers.end())
			{
				_validateNew(page);
				iterator = _transfers.try_emplace(page.transferID, page, now).first;
				_reservedBytes += static_cast<std::size_t>(page.totalSize);
			}
			_insert(iterator->second, page, now);
		} catch (const PageReassemblyError &error)
		{
			if (iterator != _transfers.end())
			{
				_reservedBytes -= iterator->second.writer.size();
				_transfers.erase(iterator);
			}
			_failedTransfers.insert_or_assign(page.transferID, now);
			if (error.reason() != PageReassemblyError::Reason::Capacity)
			{
				(void)_failureLimiter.malformedPage(page.data.size() + FrameHeaderSize + PageMetadataSize, now);
				(void)_failureLimiter.failedMessage();
			}
			throw;
		}
		if (iterator->second.receivedBytes != iterator->second.writer.size())
		{
			return std::nullopt;
		}
		Message result = std::move(iterator->second.writer).build();
		_reservedBytes -= result.size();
		_transfers.erase(iterator);
		_failureLimiter.successfulMessage();
		return result;
	}

	void PageReassembler::expire(Clock::time_point now)
	{
		for (auto iterator = _transfers.begin(); iterator != _transfers.end();)
		{
			if (now - iterator->second.updated < _timeout)
			{
				++iterator;
				continue;
			}
			_reservedBytes -= iterator->second.writer.size();
			iterator = _transfers.erase(iterator);
		}
		for (auto iterator = _failedTransfers.begin(); iterator != _failedTransfers.end();)
		{
			if (now - iterator->second >= _timeout)
			{
				iterator = _failedTransfers.erase(iterator);
			}
			else
			{
				++iterator;
			}
		}
	}

	void PageReassembler::clear() noexcept
	{
		_transfers.clear();
		_failedTransfers.clear();
		_reservedBytes = 0;
		_failureLimiter.clear();
	}

	std::size_t PageReassembler::reservedBytes() const noexcept
	{
		return _reservedBytes;
	}

	std::size_t PageReassembler::pendingTransfers() const noexcept
	{
		return _transfers.size();
	}

	bool PageReassembler::failureLimitReached() const
	{
		return _failureLimiter.tripped();
	}

	void PageReassembler::configureFailureLimits(PageFailureLimiter::Config config)
	{
		_failureLimiter.configure(config);
	}

	void PageReassembler::successfulMessage()
	{
		_failureLimiter.successfulMessage();
	}
}
