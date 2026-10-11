#pragma once

#include "container/byte_stream.hpp"
#include "type/uuid.hpp"
#include <cstdint>
#include <functional>

namespace spk
{
    class ReplicationInterest
    {
        std::function<void()> _cancel;
        std::function<void(std::uint32_t, const ByteStream &)> _update;

    public:
        ReplicationInterest() = default;
        ReplicationInterest(std::function<void()> cancel, std::function<void(std::uint32_t, const ByteStream &)> update);
        ReplicationInterest(const ReplicationInterest &) = delete;
        ReplicationInterest &operator=(const ReplicationInterest &) = delete;
        ReplicationInterest(ReplicationInterest &&other) noexcept;
        ReplicationInterest &operator=(ReplicationInterest &&other) noexcept;
        ~ReplicationInterest();
        void cancel() noexcept;
        void update(std::uint32_t type, const ByteStream &parameters);
    };
}
