#include "network/internal/page_scheduler.hpp"
#include "network/internal/transfer_completion.hpp"
#include "network/internal/outgoing_message_validator.hpp"

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

	std::uint64_t PageScheduler::_enqueue(Message message, Message::Priority priority)
	{
		OutgoingMessageValidator::validate(message, priority);
		if (_pendingBytes > _maxPendingBytes || message.size() > _maxPendingBytes - _pendingBytes)
		{
			throw Exception("Outgoing pending transfer storage is saturated.");
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

	std::uint64_t PageScheduler::enqueue(Message message, Message::Priority priority)
	{
		const std::scoped_lock lock(_mutex);
		return _enqueue(std::move(message), priority);
	}

	spk::Transfer PageScheduler::enqueueTracked(Message message, Message::Priority priority, spk::Transfer transfer)
	{
		const std::scoped_lock lock(_mutex);
		const std::uint64_t id = _enqueue(std::move(message), priority);
		TransferCompletion::assignID(transfer, id);
		try
		{
			_tracked.emplace(id, transfer);
		} catch (...)
		{
			_removeQueued(id);
			throw;
		}
		return transfer;
	}

	void PageScheduler::_removeQueued(std::uint64_t id) noexcept
	{
		for (auto &queue : _transfers)
		{
			for (auto it = queue.begin(); it != queue.end(); ++it)
			{
				if (it->id != id)
				{
					continue;
				}
				_pendingBytes -= it->message.size();
				queue.erase(it);
				return;
			}
		}
	}

	bool PageScheduler::complete(std::uint64_t id, spk::Transfer::Status status, spk::Transfer::Reason reason)
	{
		if (status == spk::Transfer::Status::Pending)
		{
			return false;
		}
		std::optional<spk::Transfer> transfer;
		{
			const std::scoped_lock lock(_mutex);
			const auto it = _tracked.find(id);
			if (it == _tracked.end())
			{
				return false;
			}
			transfer = it->second;
			_tracked.erase(it);
			_removeQueued(id);
		}
		return TransferCompletion::complete(*transfer, status, reason);
	}

	std::optional<Page> PageScheduler::nextPage(std::size_t maxPayloadBytes)
	{
		if (maxPayloadBytes == 0 || maxPayloadBytes > DefaultMaximumPageBytes)
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

	bool PageScheduler::isTracked(std::uint64_t id) const noexcept
	{
		const std::scoped_lock lock(_mutex);
		return _tracked.contains(id);
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
		std::unordered_map<std::uint64_t, spk::Transfer> tracked;
		{
			const std::scoped_lock lock(_mutex);
			for (auto &queue : _transfers)
			{
				queue.clear();
			}
			_pendingBytes = 0;
			tracked.swap(_tracked);
		}
		for (auto &[id, transfer] : tracked)
		{
			(void)id;
			(void)TransferCompletion::complete(transfer, spk::Transfer::Status::Indeterminate, spk::Transfer::Reason::TransportFailure);
		}
	}
}
