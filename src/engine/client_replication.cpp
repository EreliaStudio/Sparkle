#include "engine/client_replication.hpp"
#include "engine/engine.hpp"
#include "engine/registry.hpp"
#include "exception.hpp"
#include <limits>
#include <utility>

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

    struct ClientReplication::State
    {
        Client *client;
        UUID session;
        std::map<UUID, std::pair<std::uint32_t, ByteStream>> interests;

        void send(UUID id, std::uint32_t type, const ByteStream &parameters)
        {
            if (session.isNull()) return;
            if (parameters.size() > std::numeric_limits<std::uint32_t>::max())
                throw Exception("Interest parameters too large.");
            Message::Writer writer(InterestMessage);
            writeUUID(writer.payload(), session);
            writeUUID(writer.payload(), id);
            writer.payload() << type << static_cast<std::uint32_t>(parameters.size());
            writer.payload().append(parameters.data().data(), parameters.size());
            client->send(std::move(writer).build());
        }
    };

    ClientReplication::ClientReplication(Client &client) :
        System("Client replication"), _client(client), _state(std::make_shared<State>(State{&client, {}, {}}))
    {
        _connected = client.subscribeToConnection([this] {
            _state->session = {};
        });
        _disconnected = client.subscribeToDisconnection([this] {
            _state->session = {};
            if (engine())
                for (auto *base : Registry<Component, Engine *>::instance().elements(engine()))
                    if (auto *component = dynamic_cast<ReplicatedComponent *>(base))
                        component->resetReceivedRevision();
        });
        _welcome = client.messageDispatcher().subscribeTo(WelcomeMessage,
            [this](const Message &message) { _onWelcome(message); });
        _updates = client.messageDispatcher().subscribeTo(StateMessage,
            [this](const Message &message) { _onState(message); });
    }

    void ClientReplication::_onWelcome(const Message &message)
    {
        auto reader = message.reader();
        const auto session = readUUID(reader);
        if (session.isNull() || reader.remaining()) throw Exception("Invalid replication session.");
        _state->session = session;
        if (engine())
            for (auto *base : Registry<Component, Engine *>::instance().elements(engine()))
                if (auto *component = dynamic_cast<ReplicatedComponent *>(base))
                    component->resetReceivedRevision();
        for (const auto &[id, data] : _state->interests)
            _state->send(id, data.first, data.second);
    }

    void ClientReplication::_onState(const Message &message)
    {
        auto reader = message.reader();
        if (_state->session.isNull() || readUUID(reader) != _state->session) return;
        while (reader.remaining()) {
            const auto operation = reader.get<std::uint8_t>();
            const auto id = readUUID(reader);
            ReplicatedComponent *found = nullptr;
            if (engine())
                for (auto *base : Registry<Component, Engine *>::instance().elements(engine()))
                    if (auto *component = dynamic_cast<ReplicatedComponent *>(base); component && component->id() == id) {
                        found = component;
                        break;
                    }
            if (operation == 2) {
                if (found) found->resetReceivedRevision();
                continue;
            }
            if (operation != 1) throw Exception("Invalid replication operation.");
            const auto revision = reader.get<std::uint64_t>();
            const auto length = reader.get<std::uint32_t>();
            if (length > reader.remaining()) throw Exception("Invalid replicated section length.");
            const auto begin = reader.readOffset();
            if (found) found->applyState(revision, reader.subreader(begin, begin + length));
            reader.skip(length);
        }
    }

    void ClientReplication::_updateState(UpdateContext &) { _client.treatMessages(); }

    ReplicationInterest ClientReplication::subscribe(std::uint32_t type, const ByteStream &parameters)
    {
        if (type == 0) throw Exception("Interest type zero is reserved.");
        auto state = _state;
        const auto id = UUID::generate();
        state->interests.emplace(id, std::make_pair(type, parameters));
        state->send(id, type, parameters);
        return ReplicationInterest([weak = std::weak_ptr<State>(state), id] {
            if (auto current = weak.lock()) {
                current->interests.erase(id);
                current->send(id, 0, {});
            }
        });
    }

    void ClientReplication::updateInterest(const UUID &id, std::uint32_t type, const ByteStream &parameters)
    {
        if (type == 0 || !_state->interests.contains(id))
            throw Exception("Unknown or invalid interest.");
        _state->interests[id] = {type, parameters};
        _state->send(id, type, parameters);
    }
}
