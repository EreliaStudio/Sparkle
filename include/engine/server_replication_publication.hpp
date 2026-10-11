#pragma once
#include "engine/replicated_component.hpp"
#include "engine/server_replication_interests.hpp"
#include "network/server.hpp"
#include <chrono>
#include <functional>

namespace spk
{
    class Engine;
    class ServerReplicationPublication final
    {
    public:
        using Matcher = std::function<bool(const ReplicatedComponent &, std::uint32_t, const ByteStream::Reader &)>;
        using Authorizer = std::function<bool(ConnectionID, const ReplicatedComponent &)>;
    private:
        Matcher _matches;
        Authorizer _authorizes;
        std::chrono::steady_clock::duration _interval;
        std::chrono::steady_clock::time_point _last{};
        [[nodiscard]] bool _visible(ConnectionID connection, const ServerReplicationInterests::Peer &peer,
            const ReplicatedComponent &component) const;
        void _send(Server &server, Engine *engine, ConnectionID connection, ServerReplicationInterests::Peer &peer);
    public:
        ServerReplicationPublication(Matcher matches, Authorizer authorizes, std::chrono::steady_clock::duration interval);
        void publish(Server &server, Engine *engine, ServerReplicationInterests &interests);
    };
}
