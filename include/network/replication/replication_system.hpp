#pragma once

#include "engine/system.hpp"
#include "network/client.hpp"
#include "network/server.hpp"
#include "network/replication/replicated_component.hpp"

#include <functional>
#include <map>
#include <mutex>
#include <set>

namespace spk::Network
{
	// Owns the two reserved message types and drains its transport dispatcher during update.
	// Bind before starting the server. Other message handlers may share the dispatcher.
	class ReplicationSystem final : public spk::System
	{
	public:
		using Authorizer = std::function<bool(spk::ConnectionID, const ReplicatedComponent &)>;

	private:
		spk::Client *_client = nullptr;
		spk::Server *_server = nullptr;
		spk::Message::Type _requestType;
		spk::Message::Type _stateType;
		spk::Client::MessageDispatcher::Contract _clientStateContract;
		spk::Server::MessageDispatcher::Contract _serverRequestContract;
		spk::Server::ConnectionContract _connectionContract;
		spk::Server::DisconnectionContract _disconnectionContract;
		std::mutex _peerMutex;
		std::set<spk::ConnectionID> _peers;
		std::map<spk::ConnectionID, std::map<spk::UUID, std::uint64_t>> _sent;
		std::map<spk::UUID, std::uint64_t> _received;
		Authorizer _authorizer;

		[[nodiscard]] ReplicatedComponent *find(const spk::UUID &id);
		[[nodiscard]] spk::Message stateMessage(const ReplicatedComponent &component) const;
		void sendUpdates();
		void onRequest(const spk::ReceivedMessage &incoming);
		void onState(const spk::Message &message);

	protected:
		void _updateState(spk::UpdateContext &context) override;

	public:
		explicit ReplicationSystem(spk::Client &client, spk::Message::Type requestType = 0x53504B10);
		explicit ReplicationSystem(spk::Server &server, spk::Message::Type requestType = 0x53504B10);
		~ReplicationSystem() override;

		void setRequestAuthorizer(Authorizer authorizer);
		void request(const spk::UUID &identifier);
	};
}
