#pragma once

#include <mutex>
#include <utility>

namespace spk
{
    template <typename TData>
    class DoubleBuffer
    {
    private:
        mutable std::mutex _mutex;
        TData _write;
        TData _read;

    public:
        DoubleBuffer() = default;
        DoubleBuffer(const DoubleBuffer &) = delete;
        DoubleBuffer &operator=(const DoubleBuffer &) = delete;

        template <typename TOperation>
        void modify(TOperation &&operation)
        {
            const std::scoped_lock lock(_mutex);
            std::forward<TOperation>(operation)(_write);
        }

        void swap()
        {
            const std::scoped_lock lock(_mutex);
            std::swap(_write, _read);
        }

        [[nodiscard]] TData &read() noexcept
        {
            return _read;
        }
    };
}
