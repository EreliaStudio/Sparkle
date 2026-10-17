#pragma once

#include <type_traits>

namespace spk
{
    template <typename TEnum>
        requires std::is_enum_v<TEnum>
    class Flags
    {
    public:
        using Mask = std::make_unsigned_t<std::underlying_type_t<TEnum>>;

    private:
        Mask _bits = 0;

    public:
        constexpr Flags() = default;
        constexpr Flags(TEnum flag) noexcept :
            _bits(static_cast<Mask>(flag))
        {
        }

        [[nodiscard]] constexpr bool has(TEnum flag) const noexcept
        {
            return (_bits & static_cast<Mask>(flag)) == static_cast<Mask>(flag);
        }

        [[nodiscard]] constexpr Mask raw() const noexcept
        {
            return _bits;
        }

        constexpr Flags &operator|=(TEnum flag) noexcept
        {
            _bits |= static_cast<Mask>(flag);
            return *this;
        }

        constexpr Flags &operator|=(Flags flags) noexcept
        {
            _bits |= flags._bits;
            return *this;
        }

        [[nodiscard]] friend constexpr Flags operator|(Flags lhs, TEnum rhs) noexcept
        {
            lhs |= rhs;
            return lhs;
        }

        [[nodiscard]] friend constexpr Flags operator|(Flags lhs, Flags rhs) noexcept
        {
            lhs |= rhs;
            return lhs;
        }
    };
}
