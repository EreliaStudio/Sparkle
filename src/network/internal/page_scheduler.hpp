#pragma once

#include "network/internal/page.hpp"
#include "network/transfer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace spk::NetworkInternal
{
	class PageScheduler final
	{
	private:
		struct Transfer final
		{
			Message message;
			std::uint64_t id;
			std::size_t offset = 0;
		};

		mutable std::mutex _mutex;
		std::array<std::deque<Transfer>, 3> _transfers;
		std::unordered_map<std::uint64_t, spk::Transfer> _tracked;
		std::size_t _pendingBytes = 0;
		std::size_t _maxPendingBytes;
		std::uint64_t _nextTransferID = 1;
		std::size_t _turn = 0;

		[[nodiscard]] std::size_t _nextPriority() noexcept;
		[[nodiscard]] std::uint64_t _enqueue(Message message, Message::Priority priority);
		void _removeQueued(std::uint64_t id) noexcept;

	public:
		explicit PageScheduler(std::size_t maxPendingBytes = 64u * 1024u * 1024u);

		[[nodiscard]] std::uint64_t enqueue(Message message, Message::Priority priority = Message::Priority::Normal);
		[[nodiscard]] spk::Transfer enqueueTracked(Message message, Message::Priority priority = Message::Priority::Normal);
		[[nodiscard]] bool complete(std::uint64_t id, spk::Transfer::Status status, spk::Transfer::Reason reason = spk::Transfer::Reason::None);
		[[nodiscard]] std::optional<Page> nextPage(std::size_t maxPayloadBytes);
		[[nodiscard]] std::size_t pendingBytes() const noexcept;
		[[nodiscard]] std::size_t pendingTransfers() const noexcept;
		[[nodiscard]] bool isTracked(std::uint64_t id) const noexcept;
		[[nodiscard]] std::size_t maxPendingBytes() const noexcept;
		void setMaxPendingBytes(std::size_t bytes) noexcept;
		void clear() noexcept;
	};
}
