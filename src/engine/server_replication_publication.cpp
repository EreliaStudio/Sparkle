#include "engine/server_replication_publication.hpp"
#include "engine/engine.hpp"
#include "engine/registry.hpp"
#include "exception.hpp"
#include <limits>

namespace spk
{
    namespace
    {
        constexpr Message::Type StateMessage = 0x53505242;
        void writeUUID(ByteStream::Writer &writer, const UUID &id) { writer.append(id.bytes().data(), UUID::Size); }
    }

    ServerReplicationPublication::ServerReplicationPublication(Matcher matches, Authorizer authorizes,
        std::chrono::steady_clock::duration interval) :
        _matches(std::move(matches)), _authorizes(std::move(authorizes)), _interval(interval)
    {
        if (!_matches || !_authorizes || interval < std::chrono::steady_clock::duration::zero())
            throw Exception("Invalid server replication configuration.");
    }
    bool ServerReplicationPublication::_visible(ConnectionID connection, const ServerReplicationInterests::Peer &peer,
        const ReplicatedComponent &component) const
    {
        if (!_authorizes(connection, component)) return false;
        for (const auto &[id, interest] : peer.interests)
            if (_matches(component, interest.type, interest.parameters.reader())) return true;
        return false;
    }

    void ServerReplicationPublication::_send(Server &server, Engine *engine, ConnectionID connection, ServerReplicationInterests::Peer &peer)
    {
        if (engine == nullptr) return;
        std::map<UUID, std::uint64_t> visible;
        Message::Writer writer(StateMessage);
        writeUUID(writer.payload(), peer.session);
        auto &components = Registry<Component, Engine *>::instance().elements(engine);
        for (auto *base : components) {
            auto *component = dynamic_cast<ReplicatedComponent *>(base);
            if (!component || !_visible(connection, peer, *component)) continue;
            const auto id = component->id();
            const auto revision = component->version() + 1;
            if (!visible.emplace(id, revision).second)
                throw Exception("Duplicate replicated component UUID.");
            const auto published = peer.published.find(id);
            if (published != peer.published.end() && published->second == revision) continue;
            auto state = component->saveState();
            if (state.size() > std::numeric_limits<std::uint32_t>::max())
                throw Exception("Replicated state too large.");
            writer.payload() << std::uint8_t{1};
            writeUUID(writer.payload(), id);
            writer.payload() << revision << static_cast<std::uint32_t>(state.size());
            writer.payload().append(state.data().data(), state.size());
        }
        for (const auto &[id, revision] : peer.published)
            if (!visible.contains(id)) {
                writer.payload() << std::uint8_t{2};
                writeUUID(writer.payload(), id);
            }
        if (writer.payload().size() > UUID::Size)
            server.sendTo(connection, std::move(writer).build());
        peer.published = std::move(visible);
    }


    void ServerReplicationPublication::publish(Server &server, Engine *engine, ServerReplicationInterests &interests)
    {
        const auto now = std::chrono::steady_clock::now();
        if (_last != std::chrono::steady_clock::time_point{} && now - _last < _interval) return;
        _last = now;
        for (auto &[connection, peer] : interests.peers())
            _send(server, engine, connection, peer);
    }
}
