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
		if (page.transferID == 0 || page.offset != 0 || page.totalSize > _maxMessageBytes ||
			page.totalSize > _maxIncompleteBytes - std::min(_reservedBytes, _maxIncompleteBytes) ||
			page.totalSize > std::numeric_limits<std::size_t>::max() ||
			_transfers.size() >= 512 || (page.totalSize != 0 && page.data.empty()))
		{
			throw Exception("Incoming transfer exceeds configured limits or has an invalid first page.");
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
		if ((page.totalSize != 0 && page.data.empty()) ||
			page.totalSize != transfer.writer.size() ||
			page.header.messageType != transfer.writer.type() ||
			page.header.requestID != transfer.writer.requestID() ||
			page.header.connectionID != transfer.writer.connectionID() ||
			page.offset > page.totalSize || page.data.size() > page.totalSize - page.offset)
		{
			throw Exception("Inconsistent network page metadata.");
		}
		auto next = transfer.portions.lower_bound(page.offset);
		if (next != transfer.portions.end() && next->first == page.offset)
		{
			if (!_duplicate(transfer, page))
			{
				throw Exception("Conflicting duplicate network page.");
			}
			transfer.updated = now;
			return;
		}
		if ((next != transfer.portions.end() && page.data.size() > next->first - page.offset) ||
			(next != transfer.portions.begin() &&
			 std::prev(next)->second > page.offset - std::prev(next)->first))
		{
			throw Exception("Overlapping network page.");
		}
		std::copy(page.data.begin(), page.data.end(), transfer.writer.data().begin() + page.offset);
		transfer.portions.emplace(page.offset, page.data.size());
		transfer.receivedBytes += page.data.size();
		transfer.updated = now;
	}

	std::optional<Message> PageReassembler::accept(const PageFragment &page, Clock::time_point now)
	{
		auto iterator = _transfers.find(page.transferID);
		if (iterator == _transfers.end())
		{
			_validateNew(page);
			iterator = _transfers.try_emplace(page.transferID, page, now).first;
			_reservedBytes += static_cast<std::size_t>(page.totalSize);
		}
		Transfer &transfer = iterator->second;
		try
		{
			_insert(transfer, page, now);
		} catch (...)
		{
			_reservedBytes -= transfer.writer.size();
			_transfers.erase(iterator);
			throw;
		}
		if (transfer.receivedBytes != transfer.writer.size())
		{
			return std::nullopt;
		}
		Message result = std::move(transfer.writer).build();
		_reservedBytes -= result.size();
		_transfers.erase(iterator);
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
	}

	void PageReassembler::clear() noexcept
	{
		_transfers.clear();
		_reservedBytes = 0;
	}

	std::size_t PageReassembler::reservedBytes() const noexcept
	{
		return _reservedBytes;
	}

	std::size_t PageReassembler::pendingTransfers() const noexcept
	{
		return _transfers.size();
	}
}
