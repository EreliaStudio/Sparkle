#include "engine/replication_interest.hpp"
#include <utility>
#include "exception.hpp"

namespace spk
{
    ReplicationInterest::ReplicationInterest(std::function<void()> cancel, std::function<void(std::uint32_t, const ByteStream &)> update) : _cancel(std::move(cancel)), _update(std::move(update)) {}
    ReplicationInterest::ReplicationInterest(ReplicationInterest &&other) noexcept : _cancel(std::move(other._cancel)), _update(std::move(other._update)) { other._cancel = {};
            other._update = {}; }
    ReplicationInterest &ReplicationInterest::operator=(ReplicationInterest &&other) noexcept
    {
        if (this != &other) {
            cancel();
            _cancel = std::move(other._cancel);
            _update = std::move(other._update);
            other._cancel = {};
            other._update = {};
        }
        return *this;
    }
    ReplicationInterest::~ReplicationInterest() { cancel(); }
    void ReplicationInterest::cancel() noexcept
    {
        if (!_cancel) return;
        auto action = std::move(_cancel);
        _update = {};
        try { action(); } catch (...) {}
    }
    void ReplicationInterest::update(std::uint32_t type, const ByteStream &parameters)
    {
        if (!_update) throw spk::Exception("Interest is no longer active.");
        _update(type, parameters);
    }
}
