#include "engine/client_replication.hpp"
#include "exception.hpp"
#include <limits>
#include <map>
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
            UUID::Storage bytes{};
            reader.pull(bytes.data(), bytes.size());
            return UUID(bytes);
        }
    }
    struct ClientReplication::Interests
    {
        Client *client = nullptr;
        UUID session;
        std::map<UUID, std::pair<std::uint32_t, ByteStream>> interests;

        void send(UUID id, std::uint32_t type, const ByteStream &parameters)
        {
            if (session.isNull() || !client) return;
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


    ClientReplication::ClientReplication() :
        System("Client replication"), _interests(std::make_shared<Interests>()) {}
    ClientReplication::~ClientReplication() { setClient(nullptr); }

    void ClientReplication::setClient(Client *client)
    {
        _welcome.resign();
        _updates.resign();
        _connected.resign();
        _disconnected.resign();
        _client = client;
        _interests->client = client;
        _interests->session = {};
        _receiver.reset(engine());
        if (!client) return;
        _connected = client->subscribeToConnection([this] { ++_lifecycle; });
        _disconnected = client->subscribeToDisconnection([this] { ++_lifecycle; });
        _welcome = client->messageDispatcher().subscribeTo(WelcomeMessage,
            [this](const Message &message) { _onWelcome(message); });
        _updates = client->messageDispatcher().subscribeTo(StateMessage,
            [this](const Message &message) { _receiver.receive(message, _interests->session, engine()); });
    }

    void ClientReplication::_onWelcome(const Message &message)
    {
        auto reader = message.reader();
        const auto session = readUUID(reader);
        if (session.isNull() || reader.remaining()) throw Exception("Invalid replication session.");
        _interests->session = session;
        _receiver.reset(engine());
        for (const auto &[id, data] : _interests->interests)
            _interests->send(id, data.first, data.second);
    }

    void ClientReplication::_updateState(UpdateContext &)
    {
        if (!_client) return;
        const auto lifecycle = _lifecycle.load();
        if (lifecycle != _processedLifecycle) {
            _processedLifecycle = lifecycle;
            _interests->session = {};
            _receiver.reset(engine());
        }
        _client->treatMessages();
    }
    ReplicationInterest ClientReplication::subscribe(std::uint32_t type, const ByteStream &parameters)
    {
        if (type == 0) throw Exception("Interest type zero is reserved.");
        auto state = _interests;
        const auto id = UUID::generate();
        state->interests.emplace(id, std::make_pair(type, parameters));
        state->send(id, type, parameters);
        return ReplicationInterest([weak = std::weak_ptr<Interests>(state), id] {
            if (auto current = weak.lock()) {
                current->interests.erase(id);
                current->send(id, 0, {});
            }
        }, [weak = std::weak_ptr<Interests>(state), id](std::uint32_t type, const ByteStream &parameters) {
            if (type == 0) throw Exception("Interest type zero is reserved.");
            auto current = weak.lock();
            if (!current || !current->interests.contains(id)) throw Exception("Interest expired.");
            current->interests[id] = {type, parameters};
            current->send(id, type, parameters);
        });
    }

}
