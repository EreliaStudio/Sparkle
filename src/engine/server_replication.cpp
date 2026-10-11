#include "engine/server_replication.hpp"
#include "exception.hpp"

namespace spk
{
    namespace
    {
        constexpr Message::Type InterestMessage = 0x53505241;
        constexpr Message::Type WelcomeMessage = 0x53505243;
    }
    ServerReplication::ServerReplication(Matcher matches, Authorizer authorizes,
        std::chrono::steady_clock::duration interval) :
        System("Server replication"), _publication(std::move(matches), std::move(authorizes), interval) {}

    ServerReplication::~ServerReplication() { setServer(nullptr); }

    void ServerReplication::setServer(Server *server)
    {
        _subscription.resign();
        _connected.resign();
        _disconnected.resign();
        { const std::scoped_lock lock(_eventsMutex); _events.clear(); }
        _interests.peers().clear();
        _server = server;
        if (!server) return;
        _connected = server->subscribeToConnection([this](ConnectionID id) {
            const std::scoped_lock lock(_eventsMutex);
            _events.emplace_back(id, true);
        });
        _disconnected = server->subscribeToDisconnection([this](ConnectionID id) {
            const std::scoped_lock lock(_eventsMutex);
            _events.emplace_back(id, false);
        });
        _subscription = server->messageDispatcher().subscribeTo(InterestMessage,
            [this](const ReceivedMessage &incoming) { _interests.receive(incoming); });
    }

    void ServerReplication::_processConnections()
    {
        std::vector<std::pair<ConnectionID, bool>> events;
        { const std::scoped_lock lock(_eventsMutex); events.swap(_events); }
        for (const auto &[id, connected] : events) {
            if (!connected) { _interests.disconnect(id); continue; }
            const auto session = UUID::generate();
            _interests.connect(id, session);
            Message::Writer writer(WelcomeMessage);
            writer.payload().append(session.bytes().data(), UUID::Size);
            _server->sendTo(id, std::move(writer).build());
        }
    }
    void ServerReplication::_updateState(UpdateContext &)
    {
        if (!_server) return;
        _processConnections();
        _server->treatMessages();
        _publication.publish(*_server, engine(), _interests);
    }
}
