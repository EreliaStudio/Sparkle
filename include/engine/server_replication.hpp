#pragma once

#include "engine/system.hpp"
#include "engine/replicated_component.hpp"
#include "network/server.hpp"
#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <vector>
#include <unordered_map>

namespace spk
{
    class ServerReplication final : public System
    {
    public:
        using Matcher = std::function<bool(const ReplicatedComponent &, std::uint32_t, const ByteStream::Reader &)>;
        using Authorizer = std::function<bool(ConnectionID, const ReplicatedComponent &)>;

    private:
        struct Interest { std::uint32_t type; ByteStream parameters; };
        struct Peer {
            UUID session;
            std::map<UUID, Interest> interests;
            std::map<UUID, std::uint64_t> published;
        };
        Server &_server;
        std::mutex _eventsMutex;
        std::vector<std::pair<ConnectionID, bool>> _events;
        void _processConnections();
        Matcher _matches;
        Authorizer _authorize;
        std::map<ConnectionID, Peer> _peers;
        std::chrono::steady_clock::duration _interval;
        std::chrono::steady_clock::time_point _last{};
        Server::ConnectionContract _connected;
        Server::DisconnectionContract _disconnected;
        Server::MessageDispatcher::Contract _subscriptions;

        void _onInterest(const ReceivedMessage &incoming);
        void _publish(ConnectionID connection, Peer &peer);
        [[nodiscard]] bool _visible(ConnectionID connection, const Peer &peer, const ReplicatedComponent &component) const;
        void _updateState(UpdateContext &) override;

    public:
        ServerReplication(Server &server, Matcher matches, Authorizer authorize,
            std::chrono::steady_clock::duration interval = std::chrono::milliseconds(50));
    };
}
