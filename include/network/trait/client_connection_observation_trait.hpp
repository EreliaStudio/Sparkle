#pragma once
#include "network/client.hpp"
#include <atomic>
#include <memory>
namespace spk::Network
{
	// Editions preserve reconnects even when no owner-thread treatment occurs between them.
	class ClientConnectionObservationTrait
	{
		std::shared_ptr<std::atomic<std::uint64_t>> _connectionEdition;
		std::uint64_t _observedConnection = 0;
		spk::Client::ConnectionContract _connectionContract;
		spk::Client::DisconnectionContract _disconnectionContract;

	protected:
		virtual void _onClientConnectionChanged();
		void _observeClientConnection(spk::Client &client);
		void _releaseClientObservation() noexcept;
		void _synchronizeClientConnection();
		[[nodiscard]] bool _clientConnectionObserved() const noexcept;

	public:
		ClientConnectionObservationTrait() = default;
		virtual ~ClientConnectionObservationTrait();
		ClientConnectionObservationTrait(const ClientConnectionObservationTrait &) = delete;
		ClientConnectionObservationTrait &operator=(const ClientConnectionObservationTrait &) = delete;
	};
}
