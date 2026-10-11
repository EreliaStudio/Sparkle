#include "engine/server_replication_interests.hpp"
#include "exception.hpp"
#include <utility>

namespace spk
{
    void ServerReplicationInterests::connect(ConnectionID id, UUID session)
    {
        _peers[id] = Peer{session, {}, {}};
    }
    void ServerReplicationInterests::disconnect(ConnectionID id) { _peers.erase(id); }
    std::map<ConnectionID, ServerReplicationInterests::Peer> &ServerReplicationInterests::peers() noexcept { return _peers; }
    void ServerReplicationInterests::receive(const ReceivedMessage &incoming)
    {
        const auto peer = _peers.find(incoming.emitter);
        if (peer == _peers.end()) return;
        auto reader = incoming.message.reader();
        UUID::Storage sessionBytes{};
        reader.pull(sessionBytes.data(), sessionBytes.size());
        if (UUID(sessionBytes) != peer->second.session) return;
        UUID::Storage interestBytes{};
        reader.pull(interestBytes.data(), interestBytes.size());
        const UUID id(interestBytes);
        const auto type = reader.get<std::uint32_t>();
        const auto length = reader.get<std::uint32_t>();
        if (length > reader.remaining() || length != reader.remaining())
            throw Exception("Invalid interest payload length.");
        ByteStream::Writer writer;
        writer.append(reader.data().data() + reader.readOffset(), length);
        if (type == 0) peer->second.interests.erase(id);
        else peer->second.interests[id] = Interest{type, std::move(writer).build()};
    }
}
