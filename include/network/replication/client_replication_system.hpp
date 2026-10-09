#pragma once

#include "network/replication/replication_system.hpp"
#include "network/replication/client_replicated_component.hpp"
#include "network/client.hpp"

#include <map>

namespace spk::Network
{
	class ClientReplicationSystem final : public ReplicationSystem
	{
	private:
		spk::Client &_client;
		spk::Client::MessageDispatcher::Contract _stateContract;
		std::map<spk::UUID, std::uint64_t> _received;

		void onState(const spk::Message &message);

	protected:
		void _updateState(spk::UpdateContext &) override;

	public:
		explicit ClientReplicationSystem(spk::Client &client, spk::Message::Type requestType = 0x53504B10);
		~ClientReplicationSystem() override;

		void request(const spk::UUID &identifier);
	};
}
