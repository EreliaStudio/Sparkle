#pragma once
#include "engine/system.hpp"
#include "engine/replication_interest.hpp"
#include "engine/client_replication_receiver.hpp"
#include "network/client.hpp"
#include <atomic>
#include <memory>

namespace spk
{
    class ClientReplication final : public System
    {
        struct Interests;
        Client *_client = nullptr;
        std::shared_ptr<Interests> _interests;
        ClientReplicationReceiver _receiver;
        std::atomic_uint64_t _lifecycle{0};
        std::uint64_t _processedLifecycle = 0;
        Client::ConnectionContract _connected;
        Client::DisconnectionContract _disconnected;
        Client::MessageDispatcher::Contract _welcome;
        Client::MessageDispatcher::Contract _updates;
        void _onWelcome(const Message &message);
        void _updateState(UpdateContext &) override;

    public:
        ClientReplication();
        ~ClientReplication() override;
        void setClient(Client *client);
        [[nodiscard]] ReplicationInterest subscribe(std::uint32_t type, const ByteStream &parameters);
    };
}
