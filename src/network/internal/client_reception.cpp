#include "network/internal/client_reception.hpp"

#include "exception.hpp"
#include "network/internal/page.hpp"

#include <algorithm>
#include <utility>

namespace spk::NetworkInternal
{
	ClientReception::Result ClientReception::accept(const FrameHeader &header, std::span<const std::byte> frame)
	{
		const auto kind = read16(frame.data() + 6);
		if (kind == PageFrameKind)
		{
			const auto page = decodePage(frame);
			const std::scoped_lock lock(_mutex);
			auto result = _reception.accept(page, page.header.connectionID);
			return {std::move(result.message), std::move(result.response), std::nullopt};
		}
		if (kind == TransferResponse::Codec::FrameKind)
		{
			return {std::nullopt, std::nullopt, TransferResponse::Codec::decode(frame)};
		}
		if (kind != 0)
		{
			throw Exception("Unknown network frame kind.");
		}
		Message::Writer writer(header.type);
		writer.setRequestID(header.requestID);
		writer.setConnectionID(header.connectionID);
		writer.resize(header.payloadSize);
		std::copy(frame.begin() + FrameHeaderSize, frame.end(), writer.data().begin());
		{
			const std::scoped_lock lock(_mutex);
			_reception.successfulMessage();
		}
		return {std::move(writer).build(), std::nullopt, std::nullopt};
	}

	std::vector<TransferResponse> ClientReception::expire(Connection::ID id)
	{
		const std::scoped_lock lock(_mutex);
		return _reception.expire(id);
	}

	void ClientReception::setInactivityTimeout(std::chrono::milliseconds timeout)
	{
		const std::scoped_lock lock(_mutex);
		_reception.setInactivityTimeout(timeout);
	}

	void ClientReception::setLimits(std::size_t maximumMessageBytes, std::size_t maximumIncompleteBytes)
	{
		const std::scoped_lock lock(_mutex);
		_reception.setLimits(maximumMessageBytes, maximumIncompleteBytes);
	}

	void ClientReception::configureFailureLimits(PageFailureLimits limits)
	{
		const std::scoped_lock lock(_mutex);
		_reception.configureFailureLimits(limits);
	}

	void ClientReception::clear() noexcept
	{
		const std::scoped_lock lock(_mutex);
		_reception.clear();
	}
}
