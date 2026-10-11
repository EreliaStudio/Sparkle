#pragma once
#include "engine/replicated_component.hpp"
#include "network/message.hpp"

namespace spk
{
    class Engine;
    class ClientReplicationReceiver final
    {
    public:
        void receive(const Message &message, UUID session, Engine *engine) const;
        void reset(Engine *engine) const;
    };
}
