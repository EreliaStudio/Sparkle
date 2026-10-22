#include "container/flags.hpp"
#include "design_pattern/publishable.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

namespace
{
	enum class Options : std::uint8_t
	{
		First = 1 << 0,
		Second = 1 << 1,
		Third = 1 << 2
	};

	enum class WideOptions : std::uint32_t
	{
		Low = 1u << 0,
		High = 1u << 31
	};
}

TEST(FlagsTest, DefaultFlagsAreEmpty)
{
	constexpr spk::Flags<Options> flags;
	static_assert(flags.raw() == 0);
	EXPECT_FALSE(flags.has(Options::First));
	EXPECT_FALSE(flags.has(Options::Third));
}

TEST(FlagsTest, SingleFlagOnlyEnablesItsOwnBit)
{
	constexpr spk::Flags<Options> flags{Options::Second};
	static_assert(flags.raw() == 2);
	EXPECT_TRUE(flags.has(Options::Second));
	EXPECT_FALSE(flags.has(Options::First));
	EXPECT_FALSE(flags.has(Options::Third));
}

TEST(FlagsTest, BitwiseOrCombinesFlagsWithoutMutatingInputs)
{
	constexpr spk::Flags<Options> first{Options::First};
	constexpr spk::Flags<Options> second{Options::Second};
	constexpr auto combined = first | second | Options::Third;
	static_assert(combined.raw() == 7);
	EXPECT_EQ(first.raw(), 1u);
	EXPECT_EQ(second.raw(), 2u);
	EXPECT_TRUE(combined.has(Options::First));
	EXPECT_TRUE(combined.has(Options::Second));
	EXPECT_TRUE(combined.has(Options::Third));
}

TEST(FlagsTest, InPlaceBitwiseOrAcceptsEnumAndMask)
{
	spk::Flags<Options> flags;
	flags |= Options::First;
	flags |= spk::Flags<Options>{Options::Third};
	EXPECT_EQ(flags.raw(), 5u);
	EXPECT_FALSE(flags.has(Options::Second));
}

TEST(FlagsTest, WideFlagsPreserveHighestBit)
{
	constexpr auto flags = spk::Flags<WideOptions>{WideOptions::Low} | WideOptions::High;
	static_assert(flags.raw() == (std::uint32_t{1} | (std::uint32_t{1} << 31)));
	EXPECT_TRUE(flags.has(WideOptions::Low));
	EXPECT_TRUE(flags.has(WideOptions::High));
}

TEST(FlagsTest, PublishableFlagCombinationWorksInNormalUserCode)
{
	using Publication = spk::Publishable::Publication;
	constexpr auto flags = Publication::Edition | Publication::Update;
	static_assert(flags.has(Publication::Edition));
	static_assert(flags.has(Publication::Update));
}
