#pragma once

#include "design_pattern/trait/versioned_trait.hpp"
#include "engine/component.hpp"
#include "container/byte_stream.hpp"
#include "type/uuid.hpp"

namespace spk
{
    class ReplicatedComponent : public Component, public VersionedTrait
    {
        UUID _id;
        std::uint64_t _receivedRevision = 0;

    protected:
        virtual void writeState(ByteStream::Writer &writer) const = 0;
        virtual void readState(const ByteStream::Reader &reader) = 0;

    public:
        explicit ReplicatedComponent(UUID id, const std::string &name = "Replicated component", Entity *owner = nullptr);
        [[nodiscard]] UUID id() const noexcept;
        [[nodiscard]] std::uint64_t receivedRevision() const noexcept;
        [[nodiscard]] ByteStream saveState() const;
        void applyState(std::uint64_t revision, const ByteStream::Reader &reader);
        void resetReceivedRevision() noexcept;
    };
}
