#pragma once

#include "network/replication/replication_system.hpp"
#include "network/replication/client_replicated_component.hpp"
#include "network/client.hpp"

#include <atomic>

namespace spk::Network
{
	class ClientReplicationSystem final : public ReplicationSystem
	{
	private:
		spk::Client *_client = nullptr;
		spk::Client::MessageDispatcher::Contract _stateContract;
		spk::Client::DisconnectionContract _disconnectionContract;
		spk::Client::ConnectionContract _connectionContract;
		std::atomic_bool _resetPending{true};

		void onState(const spk::Message &message);
		void resetReceivedRevisions();

	protected:
		void _updateState(spk::UpdateContext &) override;

	public:
		explicit ClientReplicationSystem(spk::Message::Type requestType = 0x53504B10);
		~ClientReplicationSystem() override;

		void bind(spk::Client &client);
		void unbind();
		[[nodiscard]] bool isBound() const noexcept;
		void request(const spk::UUID &identifier);
	};
}
