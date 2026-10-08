#pragma once
#include "network/server.hpp"
#include <memory>
#include <mutex>
#include <set>
namespace spk::Network
{
	// Connection callbacks retain signal state; observation hooks run on the owner thread.
	class ServerConnectionObservationTrait
	{
		struct Signals
		{
			std::mutex mutex;
			std::set<spk::ConnectionID> live;
		};
		std::shared_ptr<Signals> _signals;
		std::set<spk::ConnectionID> _observed;
		spk::Server::ConnectionContract _connectionContract;
		spk::Server::DisconnectionContract _disconnectionContract;

		void _applyServerConnections(const std::set<spk::ConnectionID> &live);

	protected:
		virtual void _onServerConnectionOpened(spk::ConnectionID);
		virtual void _onServerConnectionClosed(spk::ConnectionID);
		void _observeServerConnections(spk::Server &server);
		void _releaseServerObservation() noexcept;
		void _synchronizeServerConnections();
		[[nodiscard]] bool _connectionLive(spk::ConnectionID connection) const;

	public:
		ServerConnectionObservationTrait() = default;
		virtual ~ServerConnectionObservationTrait();
		ServerConnectionObservationTrait(const ServerConnectionObservationTrait &) = delete;
		ServerConnectionObservationTrait &operator=(const ServerConnectionObservationTrait &) = delete;
	};
}
