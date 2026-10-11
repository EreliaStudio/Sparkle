#pragma once
#include "container/byte_stream.hpp"
#include "network/types.hpp"
#include "type/uuid.hpp"
#include <map>
#include <cstdint>

namespace spk
{
    class ServerReplicationInterests final
    {
    public:
        struct Interest { std::uint32_t type; ByteStream parameters; };
        struct Peer
        {
            UUID session;
            std::map<UUID, Interest> interests;
            std::map<UUID, std::uint64_t> published;
        };
    private:
        std::map<ConnectionID, Peer> _peers;
    public:
        void connect(ConnectionID connection, UUID session);
        void disconnect(ConnectionID connection);
        void receive(const ReceivedMessage &incoming);
        [[nodiscard]] std::map<ConnectionID, Peer> &peers() noexcept;
    };
}
