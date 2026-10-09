#pragma once

#include "network/replication/replication_system.hpp"
#include "network/replication/server_replicated_component.hpp"
#include "network/server.hpp"

#include <functional>
#include <map>
#include <mutex>
#include <set>

namespace spk::Network
{
	class ServerReplicationSystem final : public ReplicationSystem
	{
	public:
		using Authorizer = std::function<bool(spk::ConnectionID, const ServerReplicatedComponent &)>;

	private:
		spk::Server &_server;
		spk::Server::MessageDispatcher::Contract _requestContract;
		spk::Server::ConnectionContract _connectionContract;
		spk::Server::DisconnectionContract _disconnectionContract;
		std::mutex _peerMutex;
		std::set<spk::ConnectionID> _peers;
		std::map<spk::ConnectionID, std::map<spk::UUID, std::uint64_t>> _sent;
		Authorizer _authorizer;

		[[nodiscard]] spk::Message stateMessage(const ServerReplicatedComponent &component) const;
		void sendUpdates();
		void onRequest(const spk::ReceivedMessage &incoming);

	protected:
		void _updateState(spk::UpdateContext &) override;

	public:
		explicit ServerReplicationSystem(spk::Server &server, spk::Message::Type requestType = 0x53504B10);
		~ServerReplicationSystem() override;

		void setRequestAuthorizer(Authorizer authorizer);
	};
}
