#include <gtest/gtest.h>

#include <cstdint>

#include "structure/container/spk_flags.hpp"

enum class Permissions : uint8_t
{
	None = 0U,
	Read = 1U << 0U,
	Write = 1U << 1U,
	Execute = 1U << 2U
};

TEST(FlagsTest, BitwiseOperatorsCombineEnumerators)
{
	Permissions combined = Permissions::Read | Permissions::Write;
	EXPECT_EQ(static_cast<uint8_t>(combined), static_cast<uint8_t>(Permissions::Read) | static_cast<uint8_t>(Permissions::Write))
		<< "Bitwise OR should combine underlying bits";

	Permissions intersection = combined & Permissions::Write;
	EXPECT_EQ(intersection, Permissions::Write) << "Bitwise AND should keep the intersecting flag";

	Permissions toggled = combined ^ Permissions::Write;
	EXPECT_EQ(toggled, Permissions::Read) << "Bitwise XOR should toggle the specified flag";

	Permissions inverted = ~Permissions::Read;
	EXPECT_NE(static_cast<uint8_t>(inverted), static_cast<uint8_t>(Permissions::Read)) << "Bitwise NOT should invert the underlying bits";
}

TEST(FlagsTest, FlagsClassTracksState)
{
	spk::Flags<Permissions> flags;
	EXPECT_EQ(flags.none(), true) << "Default constructed flags should have no bits set";

	flags |= Permissions::Read;
	EXPECT_EQ(flags.has(Permissions::Read), true) << "Flags should report when a bit is set";
	EXPECT_EQ(flags.any(), true) << "Flags with bits set should report as non-empty";

	flags |= Permissions::Write;
	EXPECT_EQ(flags.has(Permissions::Write), true) << "Flags should register additional bits";

	flags &= Permissions::Read;
	EXPECT_EQ(flags.has(Permissions::Read), true) << "Bitwise AND assignment should keep shared bits";
	EXPECT_EQ(flags.has(Permissions::Write), false) << "Bitwise AND assignment should clear non-shared bits";
}

TEST(FlagsTest, FlagsComparisonOperators)
{
	spk::Flags<Permissions> a(Permissions::Read);
	spk::Flags<Permissions> b(Permissions::Read);
	spk::Flags<Permissions> c(Permissions::Write);

	EXPECT_EQ(a == b, true) << "Flags with the same bit pattern should compare equal";
	EXPECT_EQ(a != c, true) << "Flags with different patterns should compare not equal";
	EXPECT_EQ(a == Permissions::Read, true) << "Flags should compare equal to matching enumerator";
	EXPECT_EQ(Permissions::Read == a, true) << "Symmetric comparison with enumerator should succeed";
	EXPECT_EQ(Permissions::Write != a, true) << "Inequality with differing enumerator should succeed";
}

TEST(FlagsTest, FlagsComplementOperator)
{
	spk::Flags<Permissions> flags(Permissions::Read);
	auto inverted = ~flags;
	EXPECT_EQ(inverted.has(Permissions::Read), false) << "Complement should clear previously set bits";
	EXPECT_EQ(inverted.any(), true) << "Complement should set other bits for the underlying mask";
}
