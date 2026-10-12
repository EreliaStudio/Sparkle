#pragma once

#include "network/internal/page.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <unordered_map>

namespace spk::NetworkInternal
{
	class PageReassembler final
	{
	public:
		using Clock = std::chrono::steady_clock;

	private:
		struct Transfer final
		{
			Message::Writer writer;
			std::map<std::uint64_t, std::size_t> portions;
			std::size_t receivedBytes = 0;
			Clock::time_point updated;

			explicit Transfer(const PageFragment &first, Clock::time_point now);
		};

		std::unordered_map<std::uint64_t, Transfer> _transfers;
		std::size_t _reservedBytes = 0;
		std::size_t _maxMessageBytes;
		std::size_t _maxIncompleteBytes;
		std::chrono::milliseconds _timeout;

		void _validateNew(const PageFragment &page) const;
		void _insert(Transfer &transfer, const PageFragment &page, Clock::time_point now);
		[[nodiscard]] bool _duplicate(const Transfer &transfer, const PageFragment &page) const;

	public:
		explicit PageReassembler(
			std::size_t maxMessageBytes = 128u * 1024u * 1024u,
			std::size_t maxIncompleteBytes = 512u * 1024u * 1024u,
			std::chrono::milliseconds timeout = std::chrono::seconds(60));

		[[nodiscard]] std::optional<Message> accept(const PageFragment &page, Clock::time_point now = Clock::now());
		void expire(Clock::time_point now = Clock::now());
		void clear() noexcept;
		[[nodiscard]] std::size_t reservedBytes() const noexcept;
		[[nodiscard]] std::size_t pendingTransfers() const noexcept;
	};
}
