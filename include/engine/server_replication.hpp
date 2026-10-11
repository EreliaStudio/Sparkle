#pragma once
#include "engine/system.hpp"
#include "engine/server_replication_interests.hpp"
#include "engine/server_replication_publication.hpp"
#include "network/server.hpp"
#include <chrono>
#include <mutex>
#include <vector>

namespace spk
{
    class ServerReplication final : public System
    {
        Server *_server = nullptr;
        ServerReplicationInterests _interests;
        ServerReplicationPublication _publication;
        std::mutex _eventsMutex;
        std::vector<std::pair<ConnectionID, bool>> _events;
        Server::ConnectionContract _connected;
        Server::DisconnectionContract _disconnected;
        Server::MessageDispatcher::Contract _subscription;
        void _processConnections();
        void _updateState(UpdateContext &) override;

    public:
        using Matcher = ServerReplicationPublication::Matcher;
        using Authorizer = ServerReplicationPublication::Authorizer;
        explicit ServerReplication(Matcher matcher, Authorizer authorizer,
            std::chrono::steady_clock::duration interval = std::chrono::milliseconds(50));
        void setServer(Server *server);
        ~ServerReplication() override;
    };
}
