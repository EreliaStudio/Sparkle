#include "network/trait/server_connection_observation_trait.hpp"
#include "exception.hpp"
namespace spk::Network
{
	ServerConnectionObservationTrait::~ServerConnectionObservationTrait()
	{
		_releaseServerObservation();
	}
	void ServerConnectionObservationTrait::_releaseServerObservation() noexcept
	{
		_connectionContract.resign();
		_disconnectionContract.resign();
		_signals.reset();
		_observed.clear();
	}
	void ServerConnectionObservationTrait::_observeServerConnections(spk::Server &server)
	{
		if (server.isRunning())
		{
			throw spk::Exception("Observe server connections before starting the server");
		}
		auto signals = std::make_shared<Signals>();
		auto connected = server.subscribeToConnection([signals](spk::ConnectionID id) {
			const std::scoped_lock lock(signals->mutex);
			signals->live.insert(id);
		});
		auto disconnected = server.subscribeToDisconnection([signals](spk::ConnectionID id) {
			const std::scoped_lock lock(signals->mutex);
			signals->live.erase(id);
		});
		_signals = std::move(signals);
		_observed.clear();
		_connectionContract = std::move(connected);
		_disconnectionContract = std::move(disconnected);
	}
	void ServerConnectionObservationTrait::_onServerConnectionOpened(spk::ConnectionID)
	{
	}
	void ServerConnectionObservationTrait::_onServerConnectionClosed(spk::ConnectionID)
	{
	}
	bool ServerConnectionObservationTrait::_connectionLive(spk::ConnectionID connection) const
	{
		if (!_signals || !_connectionContract.isValid())
		{
			return false;
		}
		const std::scoped_lock lock(_signals->mutex);
		return _signals->live.contains(connection);
	}
	void ServerConnectionObservationTrait::_synchronizeServerConnections()
	{
		if (!_signals)
		{
			return;
		}
		std::set<spk::ConnectionID> live;
		{
			const std::scoped_lock lock(_signals->mutex);
			live = _signals->live;
		}
		_applyServerConnections(live);
	}
	void ServerConnectionObservationTrait::_applyServerConnections(const std::set<spk::ConnectionID> &live)
	{
		std::erase_if(_observed, [&](spk::ConnectionID connection) {
			if (live.contains(connection))
			{
				return false;
			}
			_onServerConnectionClosed(connection);
			return true;
		});
		for (auto connection : live)
		{
			if (!_observed.contains(connection))
			{
				_onServerConnectionOpened(connection);
				_observed.insert(connection);
			}
		}
	}
}
