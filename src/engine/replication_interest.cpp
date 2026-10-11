#include "engine/replication_interest.hpp"
#include <utility>

namespace spk
{
    ReplicationInterest::ReplicationInterest(std::function<void()> cancel) : _cancel(std::move(cancel)) {}
    ReplicationInterest::ReplicationInterest(ReplicationInterest &&other) noexcept : _cancel(std::move(other._cancel)) { other._cancel = {}; }
    ReplicationInterest &ReplicationInterest::operator=(ReplicationInterest &&other) noexcept
    {
        if (this != &other) {
            cancel();
            _cancel = std::move(other._cancel);
            other._cancel = {};
        }
        return *this;
    }
    ReplicationInterest::~ReplicationInterest() { cancel(); }
    void ReplicationInterest::cancel() noexcept
    {
        if (!_cancel) return;
        auto action = std::move(_cancel);
        try { action(); } catch (...) {}
    }
}
