#include <gtest/gtest.h>

#include "type/alignment.hpp"

#include <sstream>
#include <string>

TEST(AlignmentTest, HorizontalToStringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toString(spk::Alignment::Horizontal::Left), "Left");
	EXPECT_EQ(spk::toString(spk::Alignment::Horizontal::Center), "Center");
	EXPECT_EQ(spk::toString(spk::Alignment::Horizontal::Right), "Right");
}

TEST(AlignmentTest, HorizontalToWStringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toWString(spk::Alignment::Horizontal::Left), L"Left");
	EXPECT_EQ(spk::toWString(spk::Alignment::Horizontal::Center), L"Center");
	EXPECT_EQ(spk::toWString(spk::Alignment::Horizontal::Right), L"Right");
}

TEST(AlignmentTest, VerticalToStringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toString(spk::Alignment::Vertical::Top), "Top");
	EXPECT_EQ(spk::toString(spk::Alignment::Vertical::Center), "Center");
	EXPECT_EQ(spk::toString(spk::Alignment::Vertical::Bottom), "Bottom");
}

TEST(AlignmentTest, VerticalToWStringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toWString(spk::Alignment::Vertical::Top), L"Top");
	EXPECT_EQ(spk::toWString(spk::Alignment::Vertical::Center), L"Center");
	EXPECT_EQ(spk::toWString(spk::Alignment::Vertical::Bottom), L"Bottom");
}

TEST(AlignmentTest, DefaultValueIsLeftTop)
{
	const spk::Alignment alignment;

	EXPECT_EQ(alignment.horizontal, spk::Alignment::Horizontal::Left);
	EXPECT_EQ(alignment.vertical, spk::Alignment::Vertical::Top);
}

TEST(AlignmentTest, EqualityComparesHorizontalAndVerticalValues)
{
	const spk::Alignment first{
		spk::Alignment::Horizontal::Center,
		spk::Alignment::Vertical::Bottom
	};

	const spk::Alignment same{
		spk::Alignment::Horizontal::Center,
		spk::Alignment::Vertical::Bottom
	};

	const spk::Alignment differentHorizontal{
		spk::Alignment::Horizontal::Left,
		spk::Alignment::Vertical::Bottom
	};

	const spk::Alignment differentVertical{
		spk::Alignment::Horizontal::Center,
		spk::Alignment::Vertical::Top
	};

	EXPECT_EQ(first, same);
	EXPECT_NE(first, differentHorizontal);
	EXPECT_NE(first, differentVertical);
}

TEST(AlignmentTest, ToStringReturnsExpectedValue)
{
	const spk::Alignment alignment{
		spk::Alignment::Horizontal::Center,
		spk::Alignment::Vertical::Bottom
	};

	EXPECT_EQ(spk::toString(alignment), "(Center, Bottom)");
}

TEST(AlignmentTest, ToWStringReturnsExpectedValue)
{
	const spk::Alignment alignment{
		spk::Alignment::Horizontal::Center,
		spk::Alignment::Vertical::Bottom
	};

	EXPECT_EQ(spk::toWString(alignment), L"(Center, Bottom)");
}

TEST(AlignmentTest, HorizontalOutputStreamReturnsExpectedValues)
{
	std::ostringstream stream;

	stream << spk::Alignment::Horizontal::Left << ' '
		   << spk::Alignment::Horizontal::Center << ' '
		   << spk::Alignment::Horizontal::Right;

	EXPECT_EQ(stream.str(), "Left Center Right");
}

TEST(AlignmentTest, HorizontalWideOutputStreamReturnsExpectedValues)
{
	std::wostringstream stream;

	stream << spk::Alignment::Horizontal::Left << L' '
		   << spk::Alignment::Horizontal::Center << L' '
		   << spk::Alignment::Horizontal::Right;

	EXPECT_EQ(stream.str(), L"Left Center Right");
}

TEST(AlignmentTest, VerticalOutputStreamReturnsExpectedValues)
{
	std::ostringstream stream;

	stream << spk::Alignment::Vertical::Top << ' '
		   << spk::Alignment::Vertical::Center << ' '
		   << spk::Alignment::Vertical::Bottom;

	EXPECT_EQ(stream.str(), "Top Center Bottom");
}

TEST(AlignmentTest, VerticalWideOutputStreamReturnsExpectedValues)
{
	std::wostringstream stream;

	stream << spk::Alignment::Vertical::Top << L' '
		   << spk::Alignment::Vertical::Center << L' '
		   << spk::Alignment::Vertical::Bottom;

	EXPECT_EQ(stream.str(), L"Top Center Bottom");
}

TEST(AlignmentTest, OutputStreamReturnsExpectedValue)
{
	const spk::Alignment alignment{
		spk::Alignment::Horizontal::Center,
		spk::Alignment::Vertical::Bottom
	};

	std::ostringstream stream;
	stream << alignment;

	EXPECT_EQ(stream.str(), "(Center, Bottom)");
}

TEST(AlignmentTest, WideOutputStreamReturnsExpectedValue)
{
	const spk::Alignment alignment{
		spk::Alignment::Horizontal::Center,
		spk::Alignment::Vertical::Bottom
	};

	std::wostringstream stream;
	stream << alignment;

	EXPECT_EQ(stream.str(), L"(Center, Bottom)");
}
