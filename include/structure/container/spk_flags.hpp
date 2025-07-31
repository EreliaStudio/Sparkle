#pragma once

#include <iostream>
#include <type_traits>

namespace spk
{
	template <typename TEnumType>
	concept bitmask_enum = std::is_enum_v<TEnumType>;
}

template <spk::bitmask_enum TEnumType>
constexpr TEnumType operator|(TEnumType p_lhs, TEnumType p_rhs) noexcept
{
	using U = std::underlying_type_t<TEnumType>;
	return static_cast<TEnumType>(static_cast<U>(p_lhs) | static_cast<U>(p_rhs));
}

template <spk::bitmask_enum TEnumType>
constexpr TEnumType operator&(TEnumType p_lhs, TEnumType p_rhs) noexcept
{
	using U = std::underlying_type_t<TEnumType>;
	return static_cast<TEnumType>(static_cast<U>(p_lhs) & static_cast<U>(p_rhs));
}

template <spk::bitmask_enum TEnumType>
constexpr TEnumType operator^(TEnumType p_lhs, TEnumType p_rhs) noexcept
{
	using U = std::underlying_type_t<TEnumType>;
	return static_cast<TEnumType>(static_cast<U>(p_lhs) ^ static_cast<U>(p_rhs));
}

template <spk::bitmask_enum TEnumType>
constexpr TEnumType operator~(TEnumType p_value) noexcept
{
	using U = std::underlying_type_t<TEnumType>;
	return static_cast<TEnumType>(~static_cast<U>(p_value));
}

template <spk::bitmask_enum TEnumType>
constexpr TEnumType &operator|=(TEnumType &p_lhs, TEnumType p_rhs) noexcept
{
	p_lhs = p_lhs | p_rhs;
	return (p_lhs);
}

template <spk::bitmask_enum TEnumType>
constexpr TEnumType &operator&=(TEnumType &p_lhs, TEnumType p_rhs) noexcept
{
	p_lhs = p_lhs & p_rhs;
	return (p_lhs);
}

template <spk::bitmask_enum TEnumType>
constexpr TEnumType &operator^=(TEnumType &p_lhs, TEnumType p_rhs) noexcept
{
	p_lhs = p_lhs ^ p_rhs;
	return (p_lhs);
}

namespace spk
{
	template <bitmask_enum TFlagType>
	struct Flags
	{
		using MaskType = std::underlying_type_t<TFlagType>;
		MaskType bits{};

		constexpr Flags() = default;
		constexpr Flags(TFlagType p_flagValue) :
			bits(static_cast<MaskType>(p_flagValue))
		{
		}
		constexpr explicit Flags(MaskType p_rawFlag) :
			bits(p_rawFlag)
		{
		}

		constexpr Flags &operator|=(TFlagType p_flagValue)
		{
			bits |= static_cast<MaskType>(p_flagValue);
			return *this;
		}

		constexpr Flags &operator&=(TFlagType p_flagValue)
		{
			bits &= static_cast<MaskType>(p_flagValue);
			return *this;
		}

		constexpr Flags &operator^=(TFlagType p_flagValue)
		{
			bits ^= static_cast<MaskType>(p_flagValue);
			return *this;
		}

		constexpr bool has(TFlagType p_flagValue) const
		{
			return (bits & static_cast<MaskType>(p_flagValue)) == static_cast<MaskType>(p_flagValue);
		}

		constexpr bool any() const
		{
			return bits != 0;
		}

		constexpr bool none() const
		{
			return bits == 0;
		}

		explicit constexpr operator bool() const
		{
			return any();
		}
	};

	template <bitmask_enum TFlagType>
	constexpr Flags<TFlagType> operator|(Flags<TFlagType> p_lhs, Flags<TFlagType> p_rhs) noexcept
	{
		p_lhs.bits |= p_rhs.bits;
		return p_lhs;
	}

	template <bitmask_enum TFlagType>
	constexpr Flags<TFlagType> operator&(Flags<TFlagType> p_lhs, Flags<TFlagType> p_rhs) noexcept
	{
		p_lhs.bits &= p_rhs.bits;
		return p_lhs;
	}

	template <bitmask_enum TFlagType>
	constexpr Flags<TFlagType> operator^(Flags<TFlagType> p_lhs, Flags<TFlagType> p_rhs) noexcept
	{
		p_lhs.bits ^= p_rhs.bits;
		return p_lhs;
	}

	template <bitmask_enum TFlagType>
	constexpr Flags<TFlagType> operator~(Flags<TFlagType> p_value) noexcept
	{
		p_value.bits = ~p_value.bits;
		return p_value;
	}

	template <bitmask_enum TFlagType>
	constexpr Flags<TFlagType> operator|(Flags<TFlagType> p_lhs, TFlagType p_rhs) noexcept
	{
		p_lhs |= p_rhs;
		return p_lhs;
	}

	template <bitmask_enum TFlagType>
	constexpr Flags<TFlagType> operator|(TFlagType p_lhs, Flags<TFlagType> p_rhs) noexcept
	{
		p_rhs |= p_lhs;
		return p_rhs;
	}

	template <bitmask_enum TFlagType>
	constexpr Flags<TFlagType> operator&(Flags<TFlagType> p_lhs, TFlagType p_rhs) noexcept
	{
		p_lhs.bits &= static_cast<typename Flags<TFlagType>::MaskType>(p_rhs);
		return p_lhs;
	}

	template <bitmask_enum TFlagType>
	constexpr Flags<TFlagType> operator&(TFlagType p_lhs, Flags<TFlagType> p_rhs) noexcept
	{
		return p_rhs & p_lhs;
	}

	template <bitmask_enum TFlagType>
	constexpr bool operator==(Flags<TFlagType> p_lhs, Flags<TFlagType> p_rhs) noexcept
	{
		return p_lhs.bits == p_rhs.bits;
	}

	template <bitmask_enum TFlagType>
	constexpr bool operator!=(Flags<TFlagType> p_lhs, Flags<TFlagType> p_rhs) noexcept
	{
		return p_lhs.bits != p_rhs.bits;
	}

	template <bitmask_enum TFlagType>
	constexpr bool operator==(Flags<TFlagType> p_lhs, TFlagType p_rhs) noexcept
	{
		return p_lhs.bits == static_cast<typename Flags<TFlagType>::MaskType>(p_rhs);
	}

	template <bitmask_enum TFlagType>
	constexpr bool operator==(TFlagType p_lhs, Flags<TFlagType> p_rhs) noexcept
	{
		return p_rhs == p_lhs;
	}

	template <bitmask_enum TFlagType>
	constexpr bool operator!=(Flags<TFlagType> p_lhs, TFlagType p_rhs) noexcept
	{
		return !(p_lhs == p_rhs);
	}

	template <bitmask_enum TFlagType>
	constexpr bool operator!=(TFlagType p_lhs, Flags<TFlagType> p_rhs) noexcept
	{
		return !(p_rhs == p_lhs);
	}
}

template <spk::bitmask_enum E>
std::ostream &operator<<(std::ostream &os, spk::Flags<E> f)
{
	return os << "0x" << std::hex << f.bits;
}