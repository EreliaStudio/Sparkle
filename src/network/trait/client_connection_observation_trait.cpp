#include "network/trait/client_connection_observation_trait.hpp"
namespace spk::Network
{
	ClientConnectionObservationTrait::~ClientConnectionObservationTrait()
	{
		_releaseClientObservation();
	}
	void ClientConnectionObservationTrait::_onClientConnectionChanged()
	{
	}
	void ClientConnectionObservationTrait::_observeClientConnection(spk::Client &client)
	{
		auto edition = std::make_shared<std::atomic<std::uint64_t>>(1);
		auto connected = client.subscribeToConnection([edition] {
			++*edition;
		});
		auto disconnected = client.subscribeToDisconnection([edition] {
			++*edition;
		});
		_connectionEdition = std::move(edition);
		_observedConnection = 0;
		_connectionContract = std::move(connected);
		_disconnectionContract = std::move(disconnected);
	}
	void ClientConnectionObservationTrait::_releaseClientObservation() noexcept
	{
		_connectionContract.resign();
		_disconnectionContract.resign();
		_connectionEdition.reset();
		_observedConnection = 0;
	}
	bool ClientConnectionObservationTrait::_clientConnectionObserved() const noexcept
	{
		return _connectionContract.isValid() && _connectionEdition && _observedConnection == _connectionEdition->load();
	}
	void ClientConnectionObservationTrait::_synchronizeClientConnection()
	{
		if (!_connectionEdition)
		{
			return;
		}
		const auto edition = _connectionEdition->load();
		if (_observedConnection != edition)
		{
			_onClientConnectionChanged();
			_observedConnection = edition;
		}
	}
}
