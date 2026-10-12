#include "network/internal/page_scheduler.hpp"

#include "exception.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace spk::NetworkInternal
{
	PageScheduler::PageScheduler(std::size_t maxPendingBytes) :
		_maxPendingBytes(maxPendingBytes)
	{
	}

	std::size_t PageScheduler::_nextPriority() noexcept
	{
		constexpr std::array<std::size_t, 7> rotation{2, 2, 1, 2, 2, 1, 0};
		for (std::size_t i = 0; i < rotation.size(); ++i)
		{
			const std::size_t priority = rotation[_turn++ % rotation.size()];
			if (!_transfers[priority].empty())
			{
				return priority;
			}
		}
		return _transfers.size();
	}

	std::uint64_t PageScheduler::enqueue(Message message, Message::Priority priority)
	{
		const std::scoped_lock lock(_mutex);
		if (static_cast<std::size_t>(priority) >= _transfers.size())
		{
			throw Exception("Invalid network message priority.");
		}
		if (message.size() > 64u * 1024u * 1024u)
		{
			throw Exception("Outgoing logical message exceeds the network limit.");
		}
		if (_pendingBytes >= _maxPendingBytes)
		{
			throw Exception("Outgoing pending transfer storage is saturated.");
		}
		if (message.size() > std::numeric_limits<std::size_t>::max() - _pendingBytes)
		{
			throw Exception("Outgoing pending transfer size overflows.");
		}
		if (_nextTransferID == std::numeric_limits<std::uint64_t>::max())
		{
			throw Exception("Network transfer ID space is exhausted.");
		}
		const std::uint64_t id = _nextTransferID;
		const std::size_t bytes = message.size();
		_transfers[static_cast<std::size_t>(priority)].push_back(Transfer{std::move(message), id});
		++_nextTransferID;
		_pendingBytes += bytes;
		return id;
	}

	std::optional<Page> PageScheduler::nextPage(std::size_t maxPayloadBytes)
	{
		if (maxPayloadBytes == 0 || maxPayloadBytes > MaximumPayloadSize - PageMetadataSize)
		{
			throw Exception("Invalid network page payload capacity.");
		}
		const std::scoped_lock lock(_mutex);
		const std::size_t priority = _nextPriority();
		if (priority == _transfers.size())
		{
			return std::nullopt;
		}
		auto &queue = _transfers[priority];
		Transfer transfer = std::move(queue.front());
		queue.pop_front();
		const std::size_t remaining = transfer.message.size() - transfer.offset;
		const std::size_t length = std::min(remaining, maxPayloadBytes);
		Page page{transfer.message, transfer.id, transfer.message.size(), transfer.offset, length};
		transfer.offset += length;
		if (transfer.offset == transfer.message.size())
		{
			_pendingBytes -= transfer.message.size();
		}
		else
		{
			queue.push_back(std::move(transfer));
		}
		return page;
	}

	std::size_t PageScheduler::pendingBytes() const noexcept
	{
		const std::scoped_lock lock(_mutex);
		return _pendingBytes;
	}

	std::size_t PageScheduler::pendingTransfers() const noexcept
	{
		const std::scoped_lock lock(_mutex);
		std::size_t count = 0;
		for (const auto &queue : _transfers)
		{
			count += queue.size();
		}
		return count;
	}

	std::size_t PageScheduler::maxPendingBytes() const noexcept
	{
		const std::scoped_lock lock(_mutex);
		return _maxPendingBytes;
	}

	void PageScheduler::setMaxPendingBytes(std::size_t bytes) noexcept
	{
		const std::scoped_lock lock(_mutex);
		_maxPendingBytes = bytes;
	}

	void PageScheduler::clear() noexcept
	{
		const std::scoped_lock lock(_mutex);
		for (auto &queue : _transfers)
		{
			queue.clear();
		}
		_pendingBytes = 0;
	}
}
