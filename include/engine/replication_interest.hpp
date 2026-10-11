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

    public:
        ReplicationInterest() = default;
        explicit ReplicationInterest(std::function<void()> cancel);
        ReplicationInterest(const ReplicationInterest &) = delete;
        ReplicationInterest &operator=(const ReplicationInterest &) = delete;
        ReplicationInterest(ReplicationInterest &&other) noexcept;
        ReplicationInterest &operator=(ReplicationInterest &&other) noexcept;
        ~ReplicationInterest();
        void cancel() noexcept;
    };
}
