#include "engine/server_replication.hpp"
#include "engine/engine.hpp"
#include "engine/registry.hpp"
#include "exception.hpp"
#include <limits>
#include <set>

namespace spk
{
    namespace
    {
        constexpr Message::Type InterestMessage = 0x53505241;
        constexpr Message::Type StateMessage = 0x53505242;
        constexpr Message::Type WelcomeMessage = 0x53505243;
        void writeUUID(ByteStream::Writer &writer, const UUID &id) { writer.append(id.bytes().data(), UUID::Size); }
        UUID readUUID(const ByteStream::Reader &reader)
        {
            UUID::Storage bytes;
            reader.pull(bytes.data(), bytes.size());
            return UUID(bytes);
        }
    }

    ServerReplication::ServerReplication(Server &server, Matcher matches, Authorizer authorize,
        std::chrono::steady_clock::duration interval) :
        System("Server replication"), _server(server), _matches(std::move(matches)),
        _authorize(std::move(authorize)), _interval(interval)
    {
        if (!_matches || !_authorize || interval < std::chrono::steady_clock::duration::zero())
            throw Exception("Invalid server replication configuration.");
        _connected = _server.subscribeToConnection([this](ConnectionID connection) {
            const std::scoped_lock lock(_eventsMutex);
            _events.emplace_back(connection, true);
        });
        _disconnected = _server.subscribeToDisconnection([this](ConnectionID connection) {
            const std::scoped_lock lock(_eventsMutex);
            _events.emplace_back(connection, false);
        });
        _subscriptions = _server.messageDispatcher().subscribeTo(InterestMessage,
            [this](const ReceivedMessage &incoming) { _onInterest(incoming); });
    }

    void ServerReplication::_processConnections()
    {
        std::vector<std::pair<ConnectionID, bool>> events;
        {
            const std::scoped_lock lock(_eventsMutex);
            events.swap(_events);
        }
        for (const auto &[connection, connected] : events) {
            if (!connected) {
                _peers.erase(connection);
                continue;
            }
            auto &peer = _peers[connection];
            peer = Peer{UUID::generate(), {}, {}};
            Message::Writer welcome(WelcomeMessage);
            writeUUID(welcome.payload(), peer.session);
            _server.sendTo(connection, std::move(welcome).build());
        }
    }

    void ServerReplication::_onInterest(const ReceivedMessage &incoming)
    {
        const auto peer = _peers.find(incoming.emitter);
        if (peer == _peers.end()) return;
        auto reader = incoming.message.reader();
        const auto session = readUUID(reader);
        if (session != peer->second.session) return;
        const auto id = readUUID(reader);
        const auto type = reader.get<std::uint32_t>();
        const auto length = reader.get<std::uint32_t>();
        if (length > reader.remaining()) throw Exception("Invalid interest length.");
        const auto begin = reader.readOffset();
        auto parameters = reader.subreader(begin, begin + length);
        ByteStream::Writer copy;
        copy.append(parameters.data().data(), parameters.size());
        reader.skip(length);
        if (reader.remaining()) throw Exception("Trailing interest bytes.");
        if (type == 0) peer->second.interests.erase(id);
        else peer->second.interests[id] = Interest{type, std::move(copy).build()};
    }

    bool ServerReplication::_visible(ConnectionID connection, const Peer &peer,
        const ReplicatedComponent &component) const
    {
        if (!_authorize(connection, component)) return false;
        for (const auto &[id, interest] : peer.interests)
            if (_matches(component, interest.type, interest.parameters.reader())) return true;
        return false;
    }

    void ServerReplication::_publish(ConnectionID connection, Peer &peer)
    {
        if (engine() == nullptr) return;
        std::map<UUID, std::uint64_t> visible;
        Message::Writer writer(StateMessage);
        writeUUID(writer.payload(), peer.session);
        auto &components = Registry<Component, Engine *>::instance().elements(engine());
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
            _server.sendTo(connection, std::move(writer).build());
        peer.published = std::move(visible);
    }

    void ServerReplication::_updateState(UpdateContext &)
    {
        _processConnections();
        _server.treatMessages();
        const auto now = std::chrono::steady_clock::now();
        if (_last != std::chrono::steady_clock::time_point{} && now - _last < _interval) return;
        _last = now;
        for (auto &[connection, peer] : _peers)
            _publish(connection, peer);
    }
}
