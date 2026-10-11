#include "engine/replicated_component.hpp"

#include "design_pattern/trait/memento_trait.hpp"
#include "exception.hpp"

namespace spk
{
    ReplicatedComponent::ReplicatedComponent(UUID id, const std::string &name, Entity *owner) :
        Component(name, owner), _id(id)
    {
        if (id.isNull())
            throw Exception("Replicated component UUID cannot be null.");
    }

    UUID ReplicatedComponent::id() const noexcept { return _id; }
    std::uint64_t ReplicatedComponent::receivedRevision() const noexcept { return _receivedRevision; }

    ByteStream ReplicatedComponent::saveState() const
    {
        ByteStream::Writer writer;
        writeState(writer);
        return std::move(writer).build();
    }

    void ReplicatedComponent::applyState(std::uint64_t revision, const ByteStream::Reader &reader)
    {
        if (revision == 0 || revision <= _receivedRevision)
            return;
        auto apply = [&] {
            readState(reader);
            if (reader.remaining() != 0)
                throw Exception("Replicated component contains trailing bytes.");
        };
        if (auto *memento = dynamic_cast<MementoTrait *>(this))
            memento->transaction(apply);
        else
            apply();
        _receivedRevision = revision;
    }

    void ReplicatedComponent::resetReceivedRevision() noexcept { _receivedRevision = 0; }
}
