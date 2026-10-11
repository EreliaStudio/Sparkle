#pragma once

#include "engine/system.hpp"
#include "engine/replication_interest.hpp"
#include "engine/replicated_component.hpp"
#include "network/client.hpp"
#include <cstdint>
#include <atomic>
#include <map>
#include <memory>

namespace spk
{
    class ClientReplication final : public System
    {
        struct State;
        Client &_client;
        std::atomic_uint64_t _lifecycle{0};
        std::uint64_t _processedLifecycle = 0;
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
    };
}
