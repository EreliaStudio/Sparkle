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
		std::chrono::milliseconds timeout,
		std::shared_ptr<ReassemblyBudget> sharedBudget) :
		_sharedBudget(std::move(sharedBudget)),
		_maxMessageBytes(maxMessageBytes),
		_maxIncompleteBytes(maxIncompleteBytes),
		_timeout(timeout)
	{
		if (timeout.count() <= 0)
		{
			throw Exception("Reassembly timeout must be positive.");
		}
		setLimits(maxMessageBytes, maxIncompleteBytes);
	}

	PageReassembler::~PageReassembler()
	{
		clear();
	}

	void PageReassembler::_retire(std::uint64_t id, Clock::time_point now)
	{
		_retiredTransfers.insert_or_assign(id, now);
		if (_retiredTransfers.size() > 1024)
		{
			const auto oldest = std::min_element(
				_retiredTransfers.begin(),
				_retiredTransfers.end(),
				[](const auto &left, const auto &right) {
					return left.second < right.second;
				});
			_retiredTransfers.erase(oldest);
		}
	}

	void PageReassembler::_reserve(std::size_t bytes, std::uint64_t id)
	{
		if (_chargedBytes > _maxIncompleteBytes || bytes > _maxIncompleteBytes - _chargedBytes ||
			(_sharedBudget != nullptr && !_sharedBudget->reserve(bytes)))
		{
			throw PageReassemblyError(id, PageReassemblyError::Reason::Capacity);
		}
		_chargedBytes += bytes;
	}

	void PageReassembler::_release(std::size_t bytes) noexcept
	{
		_chargedBytes -= bytes;
		if (_sharedBudget != nullptr)
		{
			_sharedBudget->release(bytes);
		}
	}

	void PageReassembler::_validateNew(const PageFragment &page) const
	{
		using Reason = PageReassemblyError::Reason;
		if (page.transferID == 0)
		{
			throw Exception("Network page has an invalid transfer ID.");
		}
		if (page.totalSize > _maxMessageBytes ||
			page.totalSize > std::numeric_limits<std::size_t>::max() - sizeof(Transfer))
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
			page.totalSize + sizeof(Transfer) > _maxIncompleteBytes - std::min(_chargedBytes, _maxIncompleteBytes))
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
		constexpr std::size_t PageCharge = sizeof(std::pair<const std::uint64_t, std::size_t>) + 3 * sizeof(void *);
		_reserve(PageCharge, page.transferID);
		try
		{
			transfer.portions.emplace(page.offset, page.data.size());
		} catch (...)
		{
			_release(PageCharge);
			throw;
		}
		transfer.chargedBytes += PageCharge;
		std::copy(page.data.begin(), page.data.end(), transfer.writer.data().begin() + page.offset);
		transfer.receivedBytes += page.data.size();
		transfer.updated = now;
	}

	std::optional<Message> PageReassembler::accept(const PageFragment &page, Clock::time_point now)
	{
		if (_retiredTransfers.contains(page.transferID))
		{
			return std::nullopt;
		}
		auto iterator = _transfers.find(page.transferID);
		try
		{
			if (iterator == _transfers.end())
			{
				_validateNew(page);
				const std::size_t charge = static_cast<std::size_t>(page.totalSize) + sizeof(Transfer);
				_reserve(charge, page.transferID);
				try
				{
					iterator = _transfers.try_emplace(page.transferID, page, now).first;
				} catch (...)
				{
					_release(charge);
					throw;
				}
				iterator->second.chargedBytes = charge;
				_reservedBytes += static_cast<std::size_t>(page.totalSize);
			}
			_insert(iterator->second, page, now);
		} catch (const PageReassemblyError &error)
		{
			if (iterator != _transfers.end())
			{
				_reservedBytes -= iterator->second.writer.size();
				_release(iterator->second.chargedBytes);
				_transfers.erase(iterator);
			}
			_retire(page.transferID, now);
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
		_release(iterator->second.chargedBytes);
		_transfers.erase(iterator);
		_retire(page.transferID, now);
		_failureLimiter.successfulMessage();
		return result;
	}

	std::vector<std::uint64_t> PageReassembler::expire(Clock::time_point now)
	{
		std::vector<std::uint64_t> expired;
		expired.reserve(_transfers.size());
		for (auto iterator = _transfers.begin(); iterator != _transfers.end();)
		{
			if (now - iterator->second.updated < _timeout)
			{
				++iterator;
				continue;
			}
			_reservedBytes -= iterator->second.writer.size();
			_release(iterator->second.chargedBytes);
			expired.push_back(iterator->first);
			_retire(iterator->first, now);
			iterator = _transfers.erase(iterator);
		}
		for (auto iterator = _retiredTransfers.begin(); iterator != _retiredTransfers.end();)
		{
			if (now - iterator->second >= _timeout)
			{
				iterator = _retiredTransfers.erase(iterator);
			}
			else
			{
				++iterator;
			}
		}
		return expired;
	}

	void PageReassembler::setInactivityTimeout(std::chrono::milliseconds timeout)
	{
		if (timeout.count() <= 0)
		{
			throw Exception("Incoming transfer inactivity timeout must be positive.");
		}
		_timeout = timeout;
	}

	void PageReassembler::setLimits(std::size_t maximumMessageBytes, std::size_t maximumIncompleteBytes)
	{
		if (maximumMessageBytes == 0 || maximumIncompleteBytes == 0)
		{
			throw Exception("Reassembly limits must be positive.");
		}
		_maxMessageBytes = maximumMessageBytes;
		_maxIncompleteBytes = maximumIncompleteBytes;
	}

	void PageReassembler::clear() noexcept
	{
		_release(_chargedBytes);
		_chargedBytes = 0;
		_transfers.clear();
		_retiredTransfers.clear();
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
