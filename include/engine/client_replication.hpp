#pragma once

#include "engine/system.hpp"
#include "engine/replication_interest.hpp"
#include "engine/replicated_component.hpp"
#include "network/client.hpp"
#include <cstdint>
#include <map>
#include <memory>

namespace spk
{
    class ClientReplication final : public System
    {
        struct State;
        Client &_client;
        std::shared_ptr<State> _state;
        Client::ConnectionContract _connected;
        Client::DisconnectionContract _disconnected;
        Client::MessageDispatcher::Contract _welcome;
        Client::MessageDispatcher::Contract _updates;
        void _onWelcome(const Message &message);
        void _onState(const Message &message);
        void _updateState(UpdateContext &) override;

    public:
        explicit ClientReplication(Client &client);
        [[nodiscard]] ReplicationInterest subscribe(std::uint32_t type, const ByteStream &parameters);
        void updateInterest(const UUID &id, std::uint32_t type, const ByteStream &parameters);
    };
}
