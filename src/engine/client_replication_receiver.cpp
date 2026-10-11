#include "engine/client_replication_receiver.hpp"
#include "engine/engine.hpp"
#include "engine/registry.hpp"
#include "exception.hpp"

namespace spk
{
    namespace
    {
        UUID readUUID(const ByteStream::Reader &reader)
        {
            UUID::Storage bytes{};
            reader.pull(bytes.data(), bytes.size());
            return UUID(bytes);
        }
    }
    void ClientReplicationReceiver::receive(const Message &message, UUID session, Engine *engine) const
    {
        auto reader = message.reader();
        if (session.isNull() || readUUID(reader) != session) return;
        while (reader.remaining()) {
            const auto operation = reader.get<std::uint8_t>();
            const auto id = readUUID(reader);
            ReplicatedComponent *found = nullptr;
            if (engine)
                for (auto *base : Registry<Component, Engine *>::instance().elements(engine))
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


    void ClientReplicationReceiver::reset(Engine *engine) const
    {
        if (!engine) return;
        for (auto *base : Registry<Component, Engine *>::instance().elements(engine))
            if (auto *component = dynamic_cast<ReplicatedComponent *>(base))
                component->resetReceivedRevision();
    }
}
