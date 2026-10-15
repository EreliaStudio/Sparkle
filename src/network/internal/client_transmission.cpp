#include "network/internal/client_transmission.hpp"

#include "exception.hpp"
#include "network/internal/page.hpp"

#include <algorithm>
#include <utility>

namespace spk::NetworkInternal
{
	void ClientTransmission::start()
	{
		const std::scoped_lock lock(_mutex);
		_accepting = true;
	}

	Transfer ClientTransmission::send(const Message &message, Message::Priority priority, std::size_t queuedCapacity, std::size_t maxPageBytes)
	{
		std::unique_lock lock(_mutex);
		if (!_accepting)
		{
			throw Exception("Cannot send through a disconnected network client.");
		}
		_budget.setMaximum(queuedCapacity);
		_transfers.setMaxPendingBytes(queuedCapacity);
		auto reservation = OutgoingReservation::reserve(_budget, message.size(), maxPageBytes);
		Transfer transfer;
		try
		{
			transfer = _transfers.enqueue(message, priority);
			_reservations.emplace(transfer.id(), std::move(reservation));
		} catch (...)
		{
			lock.unlock();
			if (transfer.id() != Transfer::InvalidID)
			{
				(void)_transfers.fail(transfer.id(), Transfer::Status::Rejected, Transfer::Reason::QueueOverflow);
			}
			throw;
		}
		_available.notify_one();
		return transfer;
	}

	void ClientTransmission::queueResponse(const TransferResponse &response)
	{
		auto frame = TransferResponse::Codec::encode(response);
		auto reservation = _budget.reserve(frame->size(), CommandBudget::Stage::Frame);
		{
			const std::scoped_lock lock(_mutex);
			if (!_accepting)
			{
				return;
			}
			_responses.push_back(TransmissionFrame{std::move(frame), 0, 0, std::move(reservation)});
		}
		_available.notify_one();
	}

	void ClientTransmission::acknowledge(const TransferResponse &response)
	{
		(void)_transfers.acknowledge(response);
		const std::scoped_lock lock(_mutex);
		_reservations.erase(response.transferID);
	}

	std::optional<TransmissionFrame> ClientTransmission::nextFrame(const std::atomic_bool &connected, std::size_t maxPageBytes)
	{
		std::unique_lock lock(_mutex);
		_available.wait(lock, [this, &connected] {
			return !connected || !_responses.empty() || _transfers.pendingTransfers() != 0;
		});
		if (!connected)
		{
			return std::nullopt;
		}
		if (!_responses.empty())
		{
			auto frame = std::move(_responses.front());
			_responses.pop_front();
			return frame;
		}
		const auto page = _transfers.nextPage(maxPageBytes);
		if (!page.has_value())
		{
			return std::nullopt;
		}
		const auto it = _reservations.find(page->transferID);
		if (it == _reservations.end())
		{
			throw Exception("Missing client outgoing page reservation.");
		}
		auto bytes = encodePage(*page);
		const auto finalPage = page->offset + page->length == page->totalSize ? page->transferID : Transfer::InvalidID;
		return TransmissionFrame{std::move(bytes), finalPage, 0, it->second.frame, page->transferID};
	}

	void ClientTransmission::frameWritten(spk::Transfer::ID finalPage)
	{
		if (finalPage == Transfer::InvalidID)
		{
			return;
		}
		_transfers.finalPageWritten(finalPage);
		const std::scoped_lock lock(_mutex);
		_reservations.erase(finalPage);
	}

	void ClientTransmission::expire()
	{
		_transfers.expire();
		const std::scoped_lock lock(_mutex);
		for (auto it = _reservations.begin(); it != _reservations.end();)
		{
			if (!_transfers.isTracked(it->first))
			{
				it = _reservations.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void ClientTransmission::setMaxMessageBytes(std::size_t bytes)
	{
		const std::scoped_lock lock(_mutex);
		_transfers.setMaxMessageBytes(bytes);
	}

	void ClientTransmission::setAcknowledgmentTimeout(std::chrono::milliseconds timeout)
	{
		_transfers.setAcknowledgmentTimeout(timeout);
	}

	void ClientTransmission::wake() noexcept
	{
		_available.notify_all();
	}

	void ClientTransmission::clear() noexcept
	{
		_transfers.clear();
		const std::scoped_lock lock(_mutex);
		_accepting = false;
		_responses.clear();
		_reservations.clear();
	}
}
